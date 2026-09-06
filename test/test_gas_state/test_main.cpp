#include <cassert>
#include <iostream>

#include "GasState.h"

int main() {
  gasmonitor::GasState state;
  const auto initial = state.snapshot(0);
  assert(!initial.sensorOnline);
  assert(initial.level == gasmonitor::HchoLevel::Unknown);

  state.observe({100, 60, 420}, 1000);
  const auto first = state.snapshot(1000);
  assert(first.sensorOnline);
  assert(first.hchoUgM3 == 60);
  assert(first.level == gasmonitor::HchoLevel::BelowReference);
  assert(first.validFrames == 1);

  state.observe({200, 140, 500}, 2000);
  state.noteChecksumError();
  const auto smoothed = state.snapshot(2000);
  assert(smoothed.tvocUgM3 == 125);
  assert(smoothed.hchoUgM3 == 80);
  assert(smoothed.eco2Ppm == 440);
  assert(smoothed.level == gasmonitor::HchoLevel::AtOrAboveReference);
  assert(smoothed.validFrames == 2);
  assert(smoothed.checksumErrors == 1);

  const auto stale = state.snapshot(5001);
  assert(!stale.sensorOnline);
  assert(stale.level == gasmonitor::HchoLevel::Unknown);
  std::cout << "Gas state behavior passed\n";
  return 0;
}
