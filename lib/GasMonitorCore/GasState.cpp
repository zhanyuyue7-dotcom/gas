#include "GasState.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gasmonitor {

namespace {

constexpr float kEmaAlpha = 0.25F;

std::uint16_t roundedU16(const float value) {
  const float clamped = std::clamp(
      value, 0.0F,
      static_cast<float>(std::numeric_limits<std::uint16_t>::max()));
  return static_cast<std::uint16_t>(std::lround(clamped));
}

float updateEma(const float previous, const std::uint16_t sample) {
  return previous * (1.0F - kEmaAlpha) +
         static_cast<float>(sample) * kEmaAlpha;
}

}  // namespace

const char* hchoLevelName(const HchoLevel level) {
  switch (level) {
    case HchoLevel::BelowReference:
      return "below_reference";
    case HchoLevel::AtOrAboveReference:
      return "at_or_above_reference";
    case HchoLevel::Unknown:
    default:
      return "unknown";
  }
}

void GasState::observe(const GasReading& reading, const std::uint64_t nowMs) {
  if (!hasReading_) {
    tvocUgM3_ = static_cast<float>(reading.tvocUgM3);
    hchoUgM3_ = static_cast<float>(reading.hchoUgM3);
    eco2Ppm_ = static_cast<float>(reading.eco2Ppm);
    hasReading_ = true;
  } else {
    tvocUgM3_ = updateEma(tvocUgM3_, reading.tvocUgM3);
    hchoUgM3_ = updateEma(hchoUgM3_, reading.hchoUgM3);
    eco2Ppm_ = updateEma(eco2Ppm_, reading.eco2Ppm);
  }
  lastFrameMs_ = nowMs;
  ++validFrames_;
}

void GasState::noteChecksumError() { ++checksumErrors_; }

GasSnapshot GasState::snapshot(const std::uint64_t nowMs) const {
  GasSnapshot result;
  result.tvocUgM3 = roundedU16(tvocUgM3_);
  result.hchoUgM3 = roundedU16(hchoUgM3_);
  result.eco2Ppm = roundedU16(eco2Ppm_);
  result.validFrames = validFrames_;
  result.checksumErrors = checksumErrors_;
  result.lastFrameMs = lastFrameMs_;
  result.sensorOnline =
      hasReading_ && nowMs >= lastFrameMs_ &&
      nowMs - lastFrameMs_ <= kSensorOfflineAfterMs;
  if (result.sensorOnline) {
    result.level = result.hchoUgM3 >= kHchoReferenceUgM3
                       ? HchoLevel::AtOrAboveReference
                       : HchoLevel::BelowReference;
  }
  return result;
}

}  // namespace gasmonitor
