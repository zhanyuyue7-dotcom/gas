#include "StatusJson.h"

namespace gasmonitor {

std::string makeStatusJson(const GasSnapshot& snapshot,
                           const std::uint64_t nowMs) {
  const std::uint64_t sampleAgeMs =
      snapshot.validFrames == 0 || nowMs < snapshot.lastFrameMs
          ? 0
          : nowMs - snapshot.lastFrameMs;
  return std::string("{\"sensorOnline\":") +
         (snapshot.sensorOnline ? "true" : "false") +
         ",\"hchoUgM3\":" + std::to_string(snapshot.hchoUgM3) +
         ",\"tvocUgM3\":" + std::to_string(snapshot.tvocUgM3) +
         ",\"eco2Ppm\":" + std::to_string(snapshot.eco2Ppm) +
         ",\"eco2Estimated\":true" +
         ",\"level\":\"" + hchoLevelName(snapshot.level) + "\"" +
         ",\"referenceUgM3\":" + std::to_string(kHchoReferenceUgM3) +
         ",\"validFrames\":" + std::to_string(snapshot.validFrames) +
         ",\"checksumErrors\":" +
         std::to_string(snapshot.checksumErrors) +
         ",\"sampleAgeMs\":" + std::to_string(sampleAgeMs) + "}";
}

}  // namespace gasmonitor
