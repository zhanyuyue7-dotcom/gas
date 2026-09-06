#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#include "OledFont.h"

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


int litPixels(const gasmonitor::OledFont font,
              const gasmonitor::OledGlyph& glyph) {
  int count = 0;
  for (int y = 0; y < glyph.height; ++y) {
    for (int x = 0; x < glyph.width; ++x) {
      if (gasmonitor::oledGlyphPixel(font, glyph, x, y)) {
        ++count;
      }
    }
  }
  return count;
}

void collectPixel(void* context, const int x, const int y) {
  auto* pixels = static_cast<std::vector<std::pair<int, int>>*>(context);
  pixels->emplace_back(x, y);
}

}  // namespace

int main() {
  using gasmonitor::OledFont;

  const char* unitCursor = "\xC2\xB5g/m\xC2\xB3";
  CHECK(gasmonitor::nextOledCodePoint(unitCursor) == 0x00B5U);
  CHECK(gasmonitor::nextOledCodePoint(unitCursor) == static_cast<std::uint32_t>('g'));
  CHECK(gasmonitor::nextOledCodePoint(unitCursor) == static_cast<std::uint32_t>('/'));
  CHECK(gasmonitor::nextOledCodePoint(unitCursor) == static_cast<std::uint32_t>('m'));
  CHECK(gasmonitor::nextOledCodePoint(unitCursor) == 0x00B3U);
  CHECK(gasmonitor::nextOledCodePoint(unitCursor) == 0U);

  const auto* micro = gasmonitor::oledGlyph(OledFont::Small, 0x00B5U);
  const auto* superscriptThree =
      gasmonitor::oledGlyph(OledFont::Small, 0x00B3U);
  const auto* subscriptTwo =
      gasmonitor::oledGlyph(OledFont::Small, 0x2082U);
  CHECK(micro != nullptr && litPixels(OledFont::Small, *micro) > 0);
  CHECK(superscriptThree != nullptr &&
         litPixels(OledFont::Small, *superscriptThree) > 0);
  CHECK(subscriptTwo != nullptr &&
         litPixels(OledFont::Small, *subscriptTwo) > 0);
  CHECK(subscriptTwo->width >= 5U);
  CHECK(subscriptTwo->height >= 6U);
  CHECK(!gasmonitor::oledGlyphPixel(OledFont::Small, *micro, -1, 0));
  CHECK(!gasmonitor::oledGlyphPixel(OledFont::Small, *micro, micro->width,
                                     0));
  CHECK(!gasmonitor::oledGlyphPixel(OledFont::Small, *micro, 0,
                                     micro->height));

  const char malformedUtf8[] = {static_cast<char>(0xC2), 'A', '\0'};
  const char* malformedCursor = malformedUtf8;
  CHECK(gasmonitor::nextOledCodePoint(malformedCursor) ==
         static_cast<std::uint32_t>('?'));
  CHECK(gasmonitor::nextOledCodePoint(malformedCursor) ==
         static_cast<std::uint32_t>('A'));

  CHECK(gasmonitor::oledTextWidth(OledFont::Large, "1") > 10);
  CHECK(gasmonitor::oledTextWidth(OledFont::Large, "2000") <= 128);
  CHECK(gasmonitor::oledTextWidth(OledFont::Medium, "5000") <= 64);
  CHECK(gasmonitor::oledTextWidth(OledFont::Small,
                                   "TVOC \xC2\xB5g/m\xC2\xB3") <= 64);
  CHECK(gasmonitor::oledTextWidth(OledFont::Small,
                                   "eCO\xE2\x82\x82 ppm") <= 64);

  std::vector<std::pair<int, int>> pixels;
  gasmonitor::renderOledTextPixels(OledFont::Small, 10, 20,
                                   "\xC2\xB5g/m\xC2\xB3", &pixels,
                                   collectPixel);
  CHECK(!pixels.empty());
  int minX = std::numeric_limits<int>::max();
  int minY = std::numeric_limits<int>::max();
  for (const auto& [x, y] : pixels) {
    if (x < minX) {
      minX = x;
    }
    if (y < minY) {
      minY = y;
    }
  }
  CHECK(minX >= 10);
  CHECK(minY == 20);

  pixels.clear();
  const int largeValueX =
      (128 - gasmonitor::oledTextWidth(OledFont::Large, "2000")) / 2;
  gasmonitor::renderOledTextPixels(OledFont::Large, largeValueX, 0, "2000",
                                   &pixels, collectPixel);
  for (const auto& [x, y] : pixels) {
    CHECK(x >= 0 && x < 128);
    CHECK(y >= 0 && y < 20);
  }

  std::cout << "OLED UTF-8 font coverage and sizing contract passed\n";
  return 0;
}

#undef CHECK

