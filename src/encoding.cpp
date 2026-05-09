#include "ssr2mihomo/encoding.h"

#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

namespace ssr2mihomo {
namespace {

std::string normalize_base64(std::string input, bool url_variant) {
    std::string normalized;
    normalized.reserve(input.size() + 4);

    for (char ch : input) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            continue;
        }

        if (url_variant) {
            if (ch == '-') {
                normalized.push_back('+');
                continue;
            }
            if (ch == '_') {
                normalized.push_back('/');
                continue;
            }
        }

        normalized.push_back(ch);
    }

    const std::size_t remainder = normalized.size() % 4;
    if (remainder != 0U) {
        normalized.append(4U - remainder, '=');
    }

    return normalized;
}

std::string decode_base64_impl(const std::string& input, bool url_variant) {
    static const std::string kAlphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const std::string normalized = normalize_base64(input, url_variant);
    std::vector<int> table(256, -1);
    for (std::size_t i = 0; i < kAlphabet.size(); ++i) {
        table[static_cast<unsigned char>(kAlphabet[i])] = static_cast<int>(i);
    }

    std::string output;
    output.reserve((normalized.size() / 4U) * 3U);

    int value = 0;
    int bits = -8;
    for (char ch : normalized) {
        if (ch == '=') {
            break;
        }

        const int decoded = table[static_cast<unsigned char>(ch)];
        if (decoded < 0) {
            throw std::runtime_error("invalid base64 input");
        }

        value = (value << 6) | decoded;
        bits += 6;
        if (bits >= 0) {
            output.push_back(static_cast<char>((value >> bits) & 0xFF));
            bits -= 8;
        }
    }

    return output;
}

}  // namespace

std::string base64_decode_loose(const std::string& input) {
    return decode_base64_impl(input, false);
}

std::string base64_url_decode_loose(const std::string& input) {
    return decode_base64_impl(input, true);
}

std::string url_decode(const std::string& input) {
    std::string output;
    output.reserve(input.size());

    for (std::size_t i = 0; i < input.size(); ++i) {
        const char ch = input[i];
        if (ch == '%' && i + 2U < input.size()) {
            const std::string hex = input.substr(i + 1U, 2U);
            char* end = nullptr;
            const long value = std::strtol(hex.c_str(), &end, 16);
            if (end == nullptr || *end != '\0') {
                throw std::runtime_error("invalid percent-encoding");
            }
            output.push_back(static_cast<char>(value));
            i += 2U;
            continue;
        }

        if (ch == '+') {
            output.push_back(' ');
            continue;
        }

        output.push_back(ch);
    }

    return output;
}

std::string trim(std::string input) {
    if (input.size() >= 3U &&
        static_cast<unsigned char>(input[0]) == 0xEF &&
        static_cast<unsigned char>(input[1]) == 0xBB &&
        static_cast<unsigned char>(input[2]) == 0xBF) {
        input.erase(0, 3);
    }

    std::size_t start = 0;
    while (start < input.size() &&
           std::isspace(static_cast<unsigned char>(input[start])) != 0) {
        ++start;
    }

    std::size_t end = input.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(input[end - 1U])) != 0) {
        --end;
    }

    return input.substr(start, end - start);
}

}  // namespace ssr2mihomo
