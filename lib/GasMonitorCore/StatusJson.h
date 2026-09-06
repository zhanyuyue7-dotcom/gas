#pragma once

#include <cstdint>
#include <string>

#include "GasState.h"

namespace gasmonitor {

std::string makeStatusJson(const GasSnapshot& snapshot, std::uint64_t nowMs);

}  // namespace gasmonitor
