#pragma once

#include <string>

#include "ssr2mihomo/types.h"

namespace ssr2mihomo {

VmessNode parse_vmess_uri(const std::string& uri);

}  // namespace ssr2mihomo
