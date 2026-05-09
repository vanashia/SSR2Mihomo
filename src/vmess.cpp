#include "ssr2mihomo/vmess.h"

#include <cctype>
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>

#include "ssr2mihomo/encoding.h"

namespace ssr2mihomo {
namespace {

void skip_ws(const std::string& s, std::size_t& i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) {
        ++i;
    }
}

void encode_utf8(unsigned int code_point, std::string& out) {
    if (code_point < 0x80U) {
        out.push_back(static_cast<char>(code_point));
    } else if (code_point < 0x800U) {
        out.push_back(static_cast<char>(0xC0U | ((code_point >> 6) & 0x1FU)));
        out.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
    } else if (code_point < 0x10000U) {
        out.push_back(static_cast<char>(0xE0U | ((code_point >> 12) & 0x0FU)));
        out.push_back(static_cast<char>(0x80U | ((code_point >> 6) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
    } else {
        out.push_back(static_cast<char>(0xF0U | ((code_point >> 18) & 0x07U)));
        out.push_back(static_cast<char>(0x80U | ((code_point >> 12) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | ((code_point >> 6) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
    }
}

unsigned int parse_hex4(const std::string& s, std::size_t pos) {
    if (pos + 4U > s.size()) {
        throw std::runtime_error("invalid \\u escape");
    }
    unsigned int value = 0U;
    for (std::size_t k = 0; k < 4U; ++k) {
        const char ch = s[pos + k];
        unsigned int digit = 0U;
        if (ch >= '0' && ch <= '9') {
            digit = static_cast<unsigned int>(ch - '0');
        } else if (ch >= 'a' && ch <= 'f') {
            digit = 10U + static_cast<unsigned int>(ch - 'a');
        } else if (ch >= 'A' && ch <= 'F') {
            digit = 10U + static_cast<unsigned int>(ch - 'A');
        } else {
            throw std::runtime_error("invalid \\u escape");
        }
        value = (value << 4) | digit;
    }
    return value;
}

std::string parse_json_string(const std::string& s, std::size_t& i) {
    if (i >= s.size() || s[i] != '"') {
        throw std::runtime_error("expected JSON string");
    }
    ++i;

    std::string out;
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\') {
            if (i + 1U >= s.size()) {
                throw std::runtime_error("dangling escape in JSON string");
            }
            const char esc = s[i + 1U];
            switch (esc) {
                case '"': out.push_back('"'); i += 2U; break;
                case '\\': out.push_back('\\'); i += 2U; break;
                case '/': out.push_back('/'); i += 2U; break;
                case 'b': out.push_back('\b'); i += 2U; break;
                case 'f': out.push_back('\f'); i += 2U; break;
                case 'n': out.push_back('\n'); i += 2U; break;
                case 'r': out.push_back('\r'); i += 2U; break;
                case 't': out.push_back('\t'); i += 2U; break;
                case 'u': {
                    const unsigned int high = parse_hex4(s, i + 2U);
                    i += 6U;
                    unsigned int code_point = high;
                    if (high >= 0xD800U && high <= 0xDBFFU) {
                        if (i + 6U > s.size() || s[i] != '\\' || s[i + 1U] != 'u') {
                            throw std::runtime_error("missing low surrogate");
                        }
                        const unsigned int low = parse_hex4(s, i + 2U);
                        if (low < 0xDC00U || low > 0xDFFFU) {
                            throw std::runtime_error("invalid low surrogate");
                        }
                        code_point = 0x10000U +
                                     (((high - 0xD800U) << 10) | (low - 0xDC00U));
                        i += 6U;
                    }
                    encode_utf8(code_point, out);
                    break;
                }
                default:
                    throw std::runtime_error("unknown JSON escape");
            }
        } else {
            out.push_back(s[i]);
            ++i;
        }
    }
    if (i >= s.size()) {
        throw std::runtime_error("unterminated JSON string");
    }
    ++i;
    return out;
}

std::string parse_json_scalar(const std::string& s, std::size_t& i) {
    skip_ws(s, i);
    if (i >= s.size()) {
        throw std::runtime_error("unexpected end of JSON input");
    }

    if (s[i] == '"') {
        return parse_json_string(s, i);
    }

    const std::size_t start = i;
    if (s[i] == 't' || s[i] == 'f' || s[i] == 'n') {
        while (i < s.size() && std::isalpha(static_cast<unsigned char>(s[i])) != 0) {
            ++i;
        }
        return s.substr(start, i - start);
    }

    if (s[i] == '-' || s[i] == '+' || (s[i] >= '0' && s[i] <= '9')) {
        ++i;
        while (i < s.size()) {
            const char ch = s[i];
            const bool is_num =
                (ch >= '0' && ch <= '9') || ch == '.' ||
                ch == 'e' || ch == 'E' || ch == '+' || ch == '-';
            if (!is_num) {
                break;
            }
            ++i;
        }
        return s.substr(start, i - start);
    }

    throw std::runtime_error("expected JSON scalar");
}

std::map<std::string, std::string> parse_flat_json_object(const std::string& s) {
    std::size_t i = 0;
    skip_ws(s, i);
    if (i >= s.size() || s[i] != '{') {
        throw std::runtime_error("vmess JSON: expected object");
    }
    ++i;
    skip_ws(s, i);

    std::map<std::string, std::string> object;
    if (i < s.size() && s[i] == '}') {
        return object;
    }

    while (i < s.size()) {
        skip_ws(s, i);
        const std::string key = parse_json_string(s, i);
        skip_ws(s, i);
        if (i >= s.size() || s[i] != ':') {
            throw std::runtime_error("vmess JSON: expected ':'");
        }
        ++i;
        const std::string value = parse_json_scalar(s, i);
        object[key] = value;

        skip_ws(s, i);
        if (i < s.size() && s[i] == ',') {
            ++i;
            continue;
        }
        if (i < s.size() && s[i] == '}') {
            return object;
        }
        throw std::runtime_error("vmess JSON: expected ',' or '}'");
    }
    throw std::runtime_error("vmess JSON: unterminated object");
}

std::string lookup(const std::map<std::string, std::string>& fields, const std::string& key) {
    const auto it = fields.find(key);
    return it == fields.end() ? std::string() : it->second;
}

int parse_int_or(const std::string& raw, int fallback) {
    if (raw.empty()) {
        return fallback;
    }
    try {
        return std::stoi(raw);
    } catch (const std::exception&) {
        throw std::runtime_error("vmess: non-integer numeric field '" + raw + "'");
    }
}

std::map<std::string, std::string> parse_query_string(const std::string& query) {
    std::map<std::string, std::string> params;
    std::size_t i = 0;
    while (i < query.size()) {
        const std::size_t amp = query.find('&', i);
        const std::size_t end = (amp == std::string::npos) ? query.size() : amp;
        const std::string pair = query.substr(i, end - i);
        const std::size_t eq = pair.find('=');
        if (!pair.empty()) {
            const std::string key = (eq == std::string::npos) ? pair : pair.substr(0, eq);
            const std::string value =
                (eq == std::string::npos) ? std::string() : url_decode(pair.substr(eq + 1U));
            params[key] = value;
        }
        if (amp == std::string::npos) {
            break;
        }
        i = amp + 1U;
    }
    return params;
}

VmessNode parse_v2rayn_json(const std::string& json) {
    const auto fields = parse_flat_json_object(json);

    VmessNode node;
    node.name = lookup(fields, "ps");
    node.server = lookup(fields, "add");

    const std::string port_raw = lookup(fields, "port");
    if (port_raw.empty()) {
        throw std::runtime_error("vmess: missing port");
    }
    node.port = parse_int_or(port_raw, 0);

    node.uuid = lookup(fields, "id");
    if (node.uuid.empty()) {
        throw std::runtime_error("vmess: missing id");
    }

    node.alter_id = parse_int_or(lookup(fields, "aid"), 0);

    const std::string scy = lookup(fields, "scy");
    if (!scy.empty()) {
        node.cipher = scy;
    }

    const std::string net = lookup(fields, "net");
    if (!net.empty()) {
        node.network = net;
    }

    const std::string tls = lookup(fields, "tls");
    node.tls = (tls == "tls" || tls == "reality");

    node.host = lookup(fields, "host");
    node.path = lookup(fields, "path");

    node.servername = lookup(fields, "sni");
    if (node.servername.empty()) {
        node.servername = node.host;
    }

    node.alpn = lookup(fields, "alpn");
    node.fingerprint = lookup(fields, "fp");

    return node;
}

VmessNode parse_shadowrocket(const std::string& userinfo, const std::string& query) {
    const std::size_t at_pos = userinfo.rfind('@');
    if (at_pos == std::string::npos) {
        throw std::runtime_error("vmess: missing '@' in Shadowrocket-format URI");
    }
    const std::string left = userinfo.substr(0, at_pos);
    const std::string right = userinfo.substr(at_pos + 1U);

    const std::size_t left_colon = left.find(':');
    if (left_colon == std::string::npos) {
        throw std::runtime_error("vmess: missing 'method:uuid' separator");
    }
    const std::size_t right_colon = right.rfind(':');
    if (right_colon == std::string::npos) {
        throw std::runtime_error("vmess: missing 'host:port' separator");
    }

    VmessNode node;
    node.cipher = left.substr(0, left_colon);
    if (node.cipher.empty()) {
        node.cipher = "auto";
    }
    node.uuid = left.substr(left_colon + 1U);
    if (node.uuid.empty()) {
        throw std::runtime_error("vmess: missing uuid");
    }
    node.server = right.substr(0, right_colon);
    node.port = parse_int_or(right.substr(right_colon + 1U), 0);
    if (node.port == 0) {
        throw std::runtime_error("vmess: invalid port");
    }

    const auto params = parse_query_string(query);
    auto get = [&params](const std::string& key) -> std::string {
        const auto it = params.find(key);
        return it == params.end() ? std::string() : it->second;
    };

    node.name = get("remarks");

    const std::string obfs = get("obfs");
    if (obfs == "websocket" || obfs == "ws") {
        node.network = "ws";
    } else if (obfs == "http") {
        node.network = "http";
    } else {
        node.network = "tcp";
    }

    node.path = get("path");
    node.host = get("obfsParam");
    if (node.host.empty()) {
        node.host = get("host");
    }

    const std::string tls_str = get("tls");
    node.tls = (tls_str == "1" || tls_str == "true");

    node.servername = get("peer");
    if (node.servername.empty()) {
        node.servername = get("sni");
    }
    if (node.servername.empty()) {
        node.servername = node.host;
    }

    const std::string aid_raw = get("alterId");
    node.alter_id = parse_int_or(aid_raw.empty() ? get("aid") : aid_raw, 0);

    return node;
}

}  // namespace

VmessNode parse_vmess_uri(const std::string& uri) {
    const std::string cleaned = trim(uri);
    if (cleaned.rfind("vmess://", 0) != 0) {
        throw std::runtime_error("entry does not start with vmess://");
    }

    const std::string body = cleaned.substr(8U);
    const std::size_t qpos = body.find('?');
    const std::string b64_part = (qpos == std::string::npos) ? body : body.substr(0, qpos);
    const std::string query = (qpos == std::string::npos) ? std::string() : body.substr(qpos + 1U);

    std::string decoded;
    try {
        decoded = base64_url_decode_loose(b64_part);
    } catch (const std::exception&) {
        decoded = base64_decode_loose(b64_part);
    }

    const std::string trimmed = trim(decoded);
    if (!trimmed.empty() && trimmed.front() == '{') {
        return parse_v2rayn_json(trimmed);
    }
    return parse_shadowrocket(decoded, query);
}

}  // namespace ssr2mihomo
