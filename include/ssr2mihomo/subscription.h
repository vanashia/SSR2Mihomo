#pragma once

#include <string>
#include <vector>

namespace ssr2mihomo {

std::vector<std::string> decode_subscription_lines(const std::string& subscription_body);

}  // namespace ssr2mihomo
