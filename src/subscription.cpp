#include "ssr2mihomo/subscription.h"

#include <sstream>
#include <stdexcept>

#include "ssr2mihomo/encoding.h"

namespace ssr2mihomo {
namespace {

std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::stringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        line = trim(line);
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    return lines;
}

}  // namespace

std::vector<std::string> decode_subscription_lines(const std::string& subscription_body) {
    const std::string cleaned = trim(subscription_body);
    if (cleaned.empty()) {
        return {};
    }

    if (cleaned.rfind("ssr://", 0) == 0) {
        return split_lines(cleaned);
    }

    try {
        return split_lines(base64_decode_loose(cleaned));
    } catch (const std::exception&) {
        return split_lines(base64_url_decode_loose(cleaned));
    }
}

}  // namespace ssr2mihomo
