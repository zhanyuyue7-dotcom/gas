#include "GasFrameParser.h"

namespace gasmonitor {

namespace {

constexpr std::uint8_t kHeader = 0x2C;
constexpr std::uint8_t kReserved = 0xE4;

std::uint16_t readBigEndian(const std::uint8_t high, const std::uint8_t low) {
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(high) << 8U) | low);
}

}  // namespace

ParseResult GasFrameParser::push(const std::uint8_t byte,
                                 GasReading& reading) {
  if (index_ == 0) {
    if (byte == kHeader) {
      frame_[index_++] = byte;
    }
    return ParseResult::None;
  }

  if (index_ == 1 && byte != kReserved) {
    index_ = byte == kHeader ? 1U : 0U;
    if (index_ == 1U) {
      frame_[0] = byte;
    }
    return ParseResult::None;
  }

  frame_[index_++] = byte;
  if (index_ < frame_.size()) {
    return ParseResult::None;
  }

  index_ = 0;
  std::uint8_t checksum = 0;
  for (std::size_t index = 0; index < frame_.size() - 1; ++index) {
    checksum = static_cast<std::uint8_t>(checksum + frame_[index]);
  }
  if (checksum != frame_.back()) {
    return ParseResult::ChecksumError;
  }

  reading.tvocUgM3 = readBigEndian(frame_[2], frame_[3]);
  reading.hchoUgM3 = readBigEndian(frame_[4], frame_[5]);
  reading.eco2Ppm = readBigEndian(frame_[6], frame_[7]);
  return ParseResult::Frame;
}

void GasFrameParser::reset() { index_ = 0; }

}  // namespace gasmonitor
