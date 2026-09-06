#include <cassert>
#include <iostream>
#include <string>

#include "StatusJson.h"

int main() {
  gasmonitor::GasSnapshot snapshot;
  snapshot.sensorOnline = true;
  snapshot.tvocUgM3 = 125;
  snapshot.hchoUgM3 = 80;
  snapshot.eco2Ppm = 440;
  snapshot.level = gasmonitor::HchoLevel::AtOrAboveReference;
  snapshot.validFrames = 12;
  snapshot.checksumErrors = 2;
  snapshot.lastFrameMs = 9000;

  const std::string json = gasmonitor::makeStatusJson(snapshot, 10000);
  assert(json ==
         "{\"sensorOnline\":true,\"hchoUgM3\":80,"
         "\"tvocUgM3\":125,\"eco2Ppm\":440,"
         "\"eco2Estimated\":true,"
         "\"level\":\"at_or_above_reference\","
         "\"referenceUgM3\":80,\"validFrames\":12,"
         "\"checksumErrors\":2,\"sampleAgeMs\":1000}");
  std::cout << "Status JSON contract passed\n";
  return 0;
}
