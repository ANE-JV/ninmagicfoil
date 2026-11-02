#pragma once

#include "ninmagic_update_policy.hpp"
#include <stop_token>
#include <switch.h>

namespace sphaira::ui { struct ProgressBox; }

namespace ninmagic::update {
// This transport verifies TLS. It does not use the general shop downloader.
auto FetchReleaseJson(std::stop_token token, std::string& out, long& http_status) -> bool;
auto Install(sphaira::ui::ProgressBox* progress, const Release& release) -> Result;
}
