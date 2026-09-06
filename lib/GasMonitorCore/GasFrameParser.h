#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gasmonitor {

struct GasReading {
  std::uint16_t tvocUgM3{0};
  std::uint16_t hchoUgM3{0};
  std::uint16_t eco2Ppm{0};
};

enum class ParseResult { None, Frame, ChecksumError };

class GasFrameParser {
 public:
  ParseResult push(std::uint8_t byte, GasReading& reading);
  void reset();

 private:
  std::array<std::uint8_t, 9> frame_{};
  std::size_t index_{0};
};

}  // namespace gasmonitor
