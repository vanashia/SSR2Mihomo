#pragma once

#include <string>

namespace ssr2mihomo {

std::string choose_user_agent(const std::string& url, const std::string& override_user_agent);
std::string fetch_url(const std::string& url,
                      bool allow_insecure,
                      const std::string& override_user_agent = "");

}  // namespace ssr2mihomo
