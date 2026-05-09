#pragma once

#include <string>
#include <vector>

#include "ssr2mihomo/types.h"

namespace ssr2mihomo {

std::string render_mihomo_config(const std::vector<SsrNode>& ssr_nodes,
                                 const std::vector<VmessNode>& vmess_nodes,
                                 const std::string& group_name);

}  // namespace ssr2mihomo
