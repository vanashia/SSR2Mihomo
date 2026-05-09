#pragma once

#include <string>
#include <vector>

namespace ssr2mihomo {

struct SsrNode {
    std::string name;
    std::string server;
    int port = 0;
    std::string cipher;
    std::string password;
    std::string protocol;
    std::string protocol_param;
    std::string obfs;
    std::string obfs_param;
};

struct VmessNode {
    std::string name;
    std::string server;
    int port = 0;
    std::string uuid;
    int alter_id = 0;
    std::string cipher = "auto";
    std::string network = "tcp";
    bool tls = false;
    std::string servername;
    std::string host;
    std::string path;
    std::string alpn;
    std::string fingerprint;
};

struct ParseResult {
    std::vector<SsrNode> nodes;
    std::vector<VmessNode> vmess_nodes;
    std::vector<std::string> warnings;
};

}  // namespace ssr2mihomo
