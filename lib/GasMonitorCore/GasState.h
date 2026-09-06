#pragma once

#include <cstdint>

#include "GasFrameParser.h"

namespace gasmonitor {

constexpr std::uint16_t kHchoReferenceUgM3 = 80;
constexpr std::uint64_t kSensorOfflineAfterMs = 3000;

enum class HchoLevel { Unknown, BelowReference, AtOrAboveReference };

const char* hchoLevelName(HchoLevel level);

struct GasSnapshot {
  bool sensorOnline{false};
  std::uint16_t tvocUgM3{0};
  std::uint16_t hchoUgM3{0};
  std::uint16_t eco2Ppm{0};
  HchoLevel level{HchoLevel::Unknown};
  std::uint32_t validFrames{0};
  std::uint32_t checksumErrors{0};
  std::uint64_t lastFrameMs{0};
};

class GasState {
 public:
  void observe(const GasReading& reading, std::uint64_t nowMs);
  void noteChecksumError();
  GasSnapshot snapshot(std::uint64_t nowMs) const;

 private:
  bool hasReading_{false};
  float tvocUgM3_{0.0F};
  float hchoUgM3_{0.0F};
  float eco2Ppm_{0.0F};
  std::uint64_t lastFrameMs_{0};
  std::uint32_t validFrames_{0};
  std::uint32_t checksumErrors_{0};
};

}  // namespace gasmonitor
