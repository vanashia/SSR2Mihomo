#include "ssr2mihomo/ssr.h"

#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "ssr2mihomo/encoding.h"
#include "ssr2mihomo/subscription.h"
#include "ssr2mihomo/vmess.h"

namespace ssr2mihomo {
namespace {

std::vector<std::string> split(const std::string& input, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream stream(input);
    std::string part;

    while (std::getline(stream, part, delimiter)) {
        parts.push_back(part);
    }

    return parts;
}

std::string decode_query_value(const std::string& value) {
    const std::string decoded = url_decode(value);
    if (decoded.empty()) {
        return {};
    }

    try {
        return base64_url_decode_loose(decoded);
    } catch (const std::exception&) {
        return base64_decode_loose(decoded);
    }
}

std::string uniquify_name(const std::string& original, std::map<std::string, int>* seen) {
    int& count = (*seen)[original];
    ++count;
    if (count == 1) {
        return original;
    }

    return original + " (" + std::to_string(count) + ")";
}

bool is_metadata_line(const std::string& entry) {
    return entry.rfind("STATUS=", 0) == 0 || entry.rfind("REMARKS=", 0) == 0;
}

std::string detect_scheme(const std::string& entry) {
    const std::size_t scheme_pos = entry.find("://");
    if (scheme_pos == std::string::npos) {
        return {};
    }
    return entry.substr(0, scheme_pos);
}

}  // namespace

SsrNode parse_ssr_uri(const std::string& uri) {
    const std::string cleaned = trim(uri);
    if (cleaned.rfind("ssr://", 0) != 0) {
        throw std::runtime_error("entry does not start with ssr://");
    }

    const std::string decoded = base64_url_decode_loose(cleaned.substr(6));
    const std::size_t query_pos = decoded.find("/?");
    const std::string main_part = decoded.substr(0, query_pos);
    const std::string query = query_pos == std::string::npos ? "" : decoded.substr(query_pos + 2U);

    const std::vector<std::string> parts = split(main_part, ':');
    if (parts.size() != 6U) {
        throw std::runtime_error("unexpected SSR core field count");
    }

    SsrNode node;
    node.server = parts[0];
    node.port = std::stoi(parts[1]);
    node.protocol = parts[2];
    node.cipher = parts[3];
    node.obfs = parts[4];
    node.password = base64_url_decode_loose(parts[5]);

    for (const std::string& pair : split(query, '&')) {
        if (pair.empty()) {
            continue;
        }

        const std::size_t equal_pos = pair.find('=');
        const std::string key = pair.substr(0, equal_pos);
        const std::string value = equal_pos == std::string::npos ? "" : pair.substr(equal_pos + 1U);

        if (key == "remarks") {
            node.name = decode_query_value(value);
        } else if (key == "obfsparam") {
            node.obfs_param = decode_query_value(value);
        } else if (key == "protoparam") {
            node.protocol_param = decode_query_value(value);
        }
    }

    return node;
}

ParseResult parse_subscription(const std::string& subscription_body) {
    ParseResult result;
    const std::vector<std::string> entries = decode_subscription_lines(subscription_body);
    std::map<std::string, int> seen_names;

    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (is_metadata_line(entries[i])) {
            continue;
        }

        const std::string scheme = detect_scheme(entries[i]);
        if (scheme == "vmess") {
            try {
                VmessNode node = parse_vmess_uri(entries[i]);
                if (node.name.empty()) {
                    node.name = "vmess-" + std::to_string(i + 1U);
                }
                node.name = uniquify_name(node.name, &seen_names);
                result.vmess_nodes.push_back(std::move(node));
            } catch (const std::exception& ex) {
                result.warnings.push_back(
                    "Skipping entry " + std::to_string(i + 1U) + ": " + ex.what());
            }
            continue;
        }

        if (!scheme.empty() && scheme != "ssr") {
            result.warnings.push_back(
                "Skipping entry " + std::to_string(i + 1U) +
                ": unsupported subscription entry " + scheme + "://");
            continue;
        }

        try {
            SsrNode node = parse_ssr_uri(entries[i]);
            if (node.name.empty()) {
                node.name = "ssr-" + std::to_string(i + 1U);
            }
            node.name = uniquify_name(node.name, &seen_names);
            result.nodes.push_back(std::move(node));
        } catch (const std::exception& ex) {
            result.warnings.push_back(
                "Skipping entry " + std::to_string(i + 1U) + ": " + ex.what());
        }
    }

    return result;
}

}  // namespace ssr2mihomo
