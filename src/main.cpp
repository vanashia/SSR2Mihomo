#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "ssr2mihomo/fetcher.h"
#include "ssr2mihomo/mihomo.h"
#include "ssr2mihomo/ssr.h"

namespace {

struct Options {
    std::string url;
    std::string output = "config.yaml";
    std::string group_name = "PROXY";
    std::string user_agent;
    bool allow_insecure = false;
};

void print_usage() {
    std::cout
        << "Usage: SSR2Mihomo --url <subscription-url> [--output config.yaml] "
           "[--group PROXY] [--user-agent UA] [--allow-insecure]\n";
}

Options parse_args(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--url" && i + 1 < argc) {
            options.url = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            options.output = argv[++i];
        } else if (arg == "--group" && i + 1 < argc) {
            options.group_name = argv[++i];
        } else if (arg == "--user-agent" && i + 1 < argc) {
            options.user_agent = argv[++i];
        } else if (arg == "--allow-insecure") {
            options.allow_insecure = true;
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            std::exit(0);
        } else {
            throw std::runtime_error("unknown or incomplete argument: " + arg);
        }
    }

    if (options.url.empty()) {
        throw std::runtime_error("missing required --url argument");
    }

    return options;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_args(argc, argv);
        const std::string raw_body =
            ssr2mihomo::fetch_url(options.url, options.allow_insecure, options.user_agent);
        const ssr2mihomo::ParseResult parsed = ssr2mihomo::parse_subscription(raw_body);
        if (parsed.nodes.empty() && parsed.vmess_nodes.empty()) {
            throw std::runtime_error("subscription did not contain any valid nodes");
        }

        const std::string yaml = ssr2mihomo::render_mihomo_config(
            parsed.nodes, parsed.vmess_nodes, options.group_name);

        std::ofstream output(options.output, std::ios::binary);
        if (!output.is_open()) {
            throw std::runtime_error("failed to open output file: " + options.output);
        }
        output << yaml;
        output.close();
        if (!output) {
            throw std::runtime_error("failed to write output file: " + options.output);
        }

        const std::size_t total_nodes = parsed.nodes.size() + parsed.vmess_nodes.size();
        std::cerr << "Parsed " << total_nodes << " node(s)";
        if (!parsed.warnings.empty()) {
            std::cerr << ", skipped " << parsed.warnings.size()
                      << " unsupported or malformed entr" 
                      << (parsed.warnings.size() == 1 ? "y" : "ies");
        }
        std::cerr << ", wrote " << options.output << '\n';
        for (const auto& warning : parsed.warnings) {
            std::cerr << "warning: " << warning << '\n';
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        print_usage();
        return 1;
    }
}
