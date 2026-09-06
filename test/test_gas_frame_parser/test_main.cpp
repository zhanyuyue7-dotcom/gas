#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

#include "GasFrameParser.h"

int main() {
  gasmonitor::GasFrameParser parser;
  gasmonitor::GasReading reading;

  const std::array<std::uint8_t, 9> frame{
      0x2C, 0xE4, 0x00, 0x7B, 0x00, 0x50, 0x01, 0xA4, 0x80};
  gasmonitor::ParseResult result = gasmonitor::ParseResult::None;
  for (const auto byte : frame) {
    result = parser.push(byte, reading);
  }

  assert(result == gasmonitor::ParseResult::Frame);
  assert(reading.tvocUgM3 == 123);
  assert(reading.hchoUgM3 == 80);
  assert(reading.eco2Ppm == 420);

  auto corrupt = frame;
  corrupt.back() = static_cast<std::uint8_t>(corrupt.back() + 1);
  result = gasmonitor::ParseResult::None;
  for (const auto byte : corrupt) {
    result = parser.push(byte, reading);
  }
  assert(result == gasmonitor::ParseResult::ChecksumError);

  const std::array<std::uint8_t, 4> garbage{0x00, 0x2C, 0x01, 0x7F};
  for (const auto byte : garbage) {
    parser.push(byte, reading);
  }
  for (const auto byte : frame) {
    result = parser.push(byte, reading);
  }
  assert(result == gasmonitor::ParseResult::Frame);
  assert(reading.hchoUgM3 == 80);
  std::cout << "TVOC-301 parsing, checksum and resync passed\n";
  return 0;
}
