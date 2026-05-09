#pragma once

#include <string>

#include "ssr2mihomo/types.h"

namespace ssr2mihomo {

ParseResult parse_subscription(const std::string& subscription_body);
SsrNode parse_ssr_uri(const std::string& uri);

}  // namespace ssr2mihomo
