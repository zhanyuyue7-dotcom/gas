#include <cstring>
#include <iostream>

#include "OledFont.h"
#include "OledLayout.h"

namespace {

[[noreturn]] void failCheck(const char* expression, const int line) {
  std::cerr << "CHECK failed at line " << line << ": " << expression << "\n";
  std::exit(1);
}

#define CHECK(condition)                                                     \
  do {                                                                       \
    if (!(condition)) {                                                      \
      failCheck(#condition, __LINE__);                                       \
    }                                                                        \
  } while (false)

}  // namespace

int main() {
  gasmonitor::GasSnapshot snapshot;
  snapshot.sensorOnline = true;
  snapshot.hchoUgM3 = 1;
  snapshot.tvocUgM3 = 11;
  snapshot.eco2Ppm = 411;
  snapshot.level = gasmonitor::HchoLevel::BelowReference;

  const auto normal = gasmonitor::makeOledLayout(snapshot);
  CHECK(normal.online);
  CHECK(std::strcmp(normal.hcho.data(), "1") == 0);
  const int normalHchoWidth =
      gasmonitor::oledTextWidth(gasmonitor::OledFont::Medium,
                                normal.hcho.data());
  const int hchoUnitWidth =
      gasmonitor::oledTextWidth(gasmonitor::OledFont::Small,
                                gasmonitor::kHchoUnitLabel);
  CHECK(normal.hchoX ==
         (128 - normalHchoWidth - gasmonitor::kHchoValueUnitGap -
          hchoUnitWidth) /
             2);
  CHECK(normal.hchoUnitX ==
         normal.hchoX + normalHchoWidth + gasmonitor::kHchoValueUnitGap);
  CHECK(std::strcmp(normal.tvoc.data(), "11") == 0);
  CHECK(std::strcmp(normal.eco2.data(), "411") == 0);
  CHECK(normal.tvocLabelX ==
         (64 - gasmonitor::oledTextWidth(gasmonitor::OledFont::Small,
                                         gasmonitor::kTvocLabel)) /
             2);
  CHECK(normal.eco2LabelX ==
         64 + (64 - gasmonitor::oledTextWidth(gasmonitor::OledFont::Small,
                                              gasmonitor::kEco2Label)) /
                  2);
  CHECK(normal.tvocX ==
         (64 - gasmonitor::oledTextWidth(gasmonitor::OledFont::Medium,
                                         normal.tvoc.data())) /
             2);
  CHECK(normal.eco2X ==
         64 + (64 - gasmonitor::oledTextWidth(gasmonitor::OledFont::Medium,
                                              normal.eco2.data())) /
                  2);

  snapshot.hchoUgM3 = 2000;
  snapshot.level = gasmonitor::HchoLevel::AtOrAboveReference;
  const auto warning = gasmonitor::makeOledLayout(snapshot);
  const int warningHchoWidth =
      gasmonitor::oledTextWidth(gasmonitor::OledFont::Medium,
                                warning.hcho.data());
  CHECK(warning.hchoX ==
         (128 - warningHchoWidth - gasmonitor::kHchoValueUnitGap -
          hchoUnitWidth) /
             2);
  CHECK(warning.hchoUnitX ==
         warning.hchoX + warningHchoWidth + gasmonitor::kHchoValueUnitGap);

  snapshot.tvocUgM3 = 5000;
  snapshot.eco2Ppm = 5000;
  const auto maximumSecondary = gasmonitor::makeOledLayout(snapshot);
  CHECK(maximumSecondary.tvocX >= 0);
  CHECK(maximumSecondary.tvocX +
             gasmonitor::oledTextWidth(gasmonitor::OledFont::Medium,
                                       maximumSecondary.tvoc.data()) <=
         64);
  CHECK(maximumSecondary.eco2X >= 64);
  CHECK(maximumSecondary.eco2X +
             gasmonitor::oledTextWidth(gasmonitor::OledFont::Medium,
                                       maximumSecondary.eco2.data()) <=
         128);

  snapshot.sensorOnline = false;
  CHECK(!gasmonitor::makeOledLayout(snapshot).online);
  std::cout << "OLED hyperlegible hierarchy and centering contract passed\n";
  return 0;
}

#undef CHECK
