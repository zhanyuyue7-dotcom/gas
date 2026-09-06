#include "OledLayout.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "OledFont.h"

namespace gasmonitor {

OledLayout makeOledLayout(const GasSnapshot& snapshot) {
  OledLayout layout;
  layout.online = snapshot.sensorOnline;
  if (!snapshot.sensorOnline) {
    return layout;
  }

  std::snprintf(layout.hcho.data(), layout.hcho.size(), "%u",
                snapshot.hchoUgM3);
  std::snprintf(layout.tvoc.data(), layout.tvoc.size(), "%u",
                snapshot.tvocUgM3);
  std::snprintf(layout.eco2.data(), layout.eco2.size(), "%u",
                snapshot.eco2Ppm);

  const int hchoWidth = oledTextWidth(OledFont::Medium, layout.hcho.data());
  const int unitWidth = oledTextWidth(OledFont::Small, kHchoUnitLabel);
  const int hchoGroupWidth = hchoWidth + kHchoValueUnitGap + unitWidth;
  layout.hchoX = std::max(0, (128 - hchoGroupWidth) / 2);
  layout.hchoUnitX = layout.hchoX + hchoWidth + kHchoValueUnitGap;
  layout.tvocLabelX =
      std::max(0, (64 - oledTextWidth(OledFont::Small, kTvocLabel)) /
                      2);
  layout.eco2LabelX =
      64 + std::max(0, (64 - oledTextWidth(OledFont::Small, kEco2Label)) /
                           2);
  layout.tvocX =
      std::max(0, (64 - oledTextWidth(OledFont::Medium,
                                      layout.tvoc.data())) /
                      2);
  layout.eco2X =
      64 + std::max(0, (64 - oledTextWidth(OledFont::Medium,
                                           layout.eco2.data())) /
                           2);
  return layout;
}

}  // namespace gasmonitor
