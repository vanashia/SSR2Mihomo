#pragma once

#include <string>

namespace ssr2mihomo {

std::string base64_decode_loose(const std::string& input);
std::string base64_url_decode_loose(const std::string& input);
std::string url_decode(const std::string& input);
std::string trim(std::string input);

}  // namespace ssr2mihomo
