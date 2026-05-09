#include "ssr2mihomo/mihomo.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

namespace ssr2mihomo {
namespace {

std::string yaml_quote(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 2U);
    escaped.push_back('\'');
    for (char ch : value) {
        if (ch == '\'') {
            escaped.append("''");
        } else {
            escaped.push_back(ch);
        }
    }
    escaped.push_back('\'');
    return escaped;
}

std::string trim_copy(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])) != 0) {
        ++start;
    }
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1U])) != 0) {
        --end;
    }
    return s.substr(start, end - start);
}

void append_ssr_proxy(std::ostringstream& out, const SsrNode& node) {
    out << "  - name: " << yaml_quote(node.name) << '\n';
    out << "    type: ssr\n";
    out << "    server: " << yaml_quote(node.server) << '\n';
    out << "    port: " << node.port << '\n';
    out << "    cipher: " << yaml_quote(node.cipher) << '\n';
    out << "    password: " << yaml_quote(node.password) << '\n';
    out << "    protocol: " << yaml_quote(node.protocol) << '\n';
    if (!node.protocol_param.empty()) {
        out << "    protocol-param: " << yaml_quote(node.protocol_param) << '\n';
    }
    out << "    obfs: " << yaml_quote(node.obfs) << '\n';
    if (!node.obfs_param.empty()) {
        out << "    obfs-param: " << yaml_quote(node.obfs_param) << '\n';
    }
    out << "    udp: true\n";
}

void append_vmess_proxy(std::ostringstream& out, const VmessNode& node) {
    out << "  - name: " << yaml_quote(node.name) << '\n';
    out << "    type: vmess\n";
    out << "    server: " << yaml_quote(node.server) << '\n';
    out << "    port: " << node.port << '\n';
    out << "    uuid: " << yaml_quote(node.uuid) << '\n';
    out << "    alterId: " << node.alter_id << '\n';
    out << "    cipher: " << yaml_quote(node.cipher) << '\n';
    out << "    udp: true\n";

    if (node.tls) {
        out << "    tls: true\n";
        if (!node.servername.empty()) {
            out << "    servername: " << yaml_quote(node.servername) << '\n';
        }
        if (!node.fingerprint.empty()) {
            out << "    client-fingerprint: " << yaml_quote(node.fingerprint) << '\n';
        }
        if (!node.alpn.empty()) {
            out << "    alpn:\n";
            std::stringstream stream(node.alpn);
            std::string item;
            while (std::getline(stream, item, ',')) {
                const std::string trimmed = trim_copy(item);
                if (!trimmed.empty()) {
                    out << "      - " << yaml_quote(trimmed) << '\n';
                }
            }
        }
    }

    if (!node.network.empty() && node.network != "tcp") {
        out << "    network: " << yaml_quote(node.network) << '\n';
    }

    if (node.network == "ws") {
        out << "    ws-opts:\n";
        out << "      path: " << yaml_quote(node.path.empty() ? "/" : node.path) << '\n';
        if (!node.host.empty()) {
            out << "      headers:\n";
            out << "        Host: " << yaml_quote(node.host) << '\n';
        }
    } else if (node.network == "grpc") {
        out << "    grpc-opts:\n";
        out << "      grpc-service-name: " << yaml_quote(node.path) << '\n';
    } else if (node.network == "h2") {
        out << "    h2-opts:\n";
        if (!node.path.empty()) {
            out << "      path: " << yaml_quote(node.path) << '\n';
        }
        if (!node.host.empty()) {
            out << "      host:\n";
            out << "        - " << yaml_quote(node.host) << '\n';
        }
    }
}

void append_ssr_names(std::ostringstream& stream,
                      const std::vector<SsrNode>& nodes,
                      int indent) {
    const std::string prefix(static_cast<std::size_t>(indent), ' ');
    for (const auto& node : nodes) {
        stream << prefix << "- " << yaml_quote(node.name) << '\n';
    }
}

void append_vmess_names(std::ostringstream& stream,
                        const std::vector<VmessNode>& nodes,
                        int indent) {
    const std::string prefix(static_cast<std::size_t>(indent), ' ');
    for (const auto& node : nodes) {
        stream << prefix << "- " << yaml_quote(node.name) << '\n';
    }
}

}  // namespace

std::string render_mihomo_config(const std::vector<SsrNode>& ssr_nodes,
                                 const std::vector<VmessNode>& vmess_nodes,
                                 const std::string& group_name) {
    if (ssr_nodes.empty() && vmess_nodes.empty()) {
        throw std::runtime_error("cannot render mihomo config with no nodes");
    }

    const std::string main_group = group_name.empty() ? "PROXY" : group_name;
    std::ostringstream out;

    out << "mixed-port: 7890\n";
    out << "allow-lan: false\n";
    out << "mode: rule\n";
    out << "log-level: info\n";
    out << '\n';

    out << "proxies:\n";
    for (const auto& node : ssr_nodes) {
        append_ssr_proxy(out, node);
    }
    for (const auto& node : vmess_nodes) {
        append_vmess_proxy(out, node);
    }
    out << '\n';

    out << "proxy-groups:\n";
    out << "  - name: " << yaml_quote(main_group) << '\n';
    out << "    type: select\n";
    out << "    proxies:\n";
    out << "      - 'AUTO'\n";
    out << "      - 'DIRECT'\n";
    append_ssr_names(out, ssr_nodes, 6);
    append_vmess_names(out, vmess_nodes, 6);
    out << "  - name: 'AUTO'\n";
    out << "    type: url-test\n";
    out << "    url: 'http://www.gstatic.com/generate_204'\n";
    out << "    interval: 300\n";
    out << "    proxies:\n";
    append_ssr_names(out, ssr_nodes, 6);
    append_vmess_names(out, vmess_nodes, 6);
    out << '\n';

    out << "rules:\n";
    out << "  - GEOIP,CN,DIRECT\n";
    out << "  - MATCH," << main_group << '\n';

    return out.str();
}

}  // namespace ssr2mihomo
