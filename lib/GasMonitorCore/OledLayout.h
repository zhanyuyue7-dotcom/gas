#pragma once

#include <array>

#include "GasState.h"

namespace gasmonitor {

inline constexpr char kHchoUnitLabel[] = "\xC2\xB5g";
inline constexpr char kTvocLabel[] = "TVOC \xC2\xB5g";
inline constexpr char kEco2Label[] = "eCO\xE2\x82\x82 ppm";
inline constexpr int kHchoValueUnitGap = 4;

struct OledLayout {
  bool online{false};
  std::array<char, 6> hcho{};
  int hchoX{0};
  int hchoUnitX{0};
  std::array<char, 6> tvoc{};
  int tvocLabelX{0};
  int tvocX{0};
  std::array<char, 6> eco2{};
  int eco2LabelX{0};
  int eco2X{0};
};

OledLayout makeOledLayout(const GasSnapshot& snapshot);

}  // namespace gasmonitor
