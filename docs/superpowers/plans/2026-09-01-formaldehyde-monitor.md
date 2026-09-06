# Formaldehyde Monitor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an ESP32-S3 monitor that parses the TVOC-301 UART stream, shows HCHO/TVOC/eCO2 on a 128×64 SSD1306 OLED, and exposes the same readings through a local Wi-Fi dashboard.

**Architecture:** Keep hardware-independent parsing, classification, smoothing, and JSON generation in `lib/GasMonitorCore` so they run in host tests. Keep UART, I2C, SSD1306, Wi-Fi, HTTP, and FreeRTOS integration in `main/main.cpp`; the web page is embedded from `main/web_page.h`.

**Tech Stack:** C++17, ESP-IDF 5.5.3, ESP32-S3, UART 9600 8N1, I2C SSD1306 128×64, CMake/Ninja, native HTML/CSS/JavaScript.

---

### Task 1: Project scaffold and host test loop

**Files:**
- Create: `CMakeLists.txt`, `sdkconfig.defaults`, `partitions.csv`
- Create: `host/CMakeLists.txt`
- Create: `lib/GasMonitorCore/CMakeLists.txt`

- [ ] **Step 1: Create ESP-IDF and host CMake targets**

```cmake
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/lib/GasMonitorCore")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(gas_monitor)
```

- [ ] **Step 2: Configure the host test loop**

Run:

```powershell
cmake -S host -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

Expected: CMake configures successfully before feature tests are added.

### Task 2: Parse TVOC-301 frames

**Files:**
- Create: `lib/GasMonitorCore/GasFrameParser.h`
- Create: `lib/GasMonitorCore/GasFrameParser.cpp`
- Create: `test/test_gas_frame_parser/test_main.cpp`

- [ ] **Step 1: Write the failing parser test**

```cpp
const std::uint8_t frame[]{0x2C, 0xE4, 0x00, 0x7B, 0x00, 0x50,
                           0x01, 0xA4, 0x86};
GasReading reading;
for (const auto byte : frame) parser.push(byte, reading);
assert(reading.tvocUgM3 == 123);
assert(reading.hchoUgM3 == 80);
assert(reading.eco2Ppm == 420);
```

- [ ] **Step 2: Run the test and verify RED**

Run: `cmake --build build-host --target test_gas_frame_parser`

Expected: compilation fails because `GasFrameParser` does not exist.

- [ ] **Step 3: Implement streaming resynchronization and checksum validation**

```cpp
enum class ParseResult { None, Frame, ChecksumError };
ParseResult push(std::uint8_t byte, GasReading& reading);
```

The checksum is the low eight bits of the sum of bytes 0 through 7. A valid frame begins with `0x2C 0xE4`; values are unsigned big-endian words.

- [ ] **Step 4: Run the parser test and verify GREEN**

Expected: valid frame, corrupt checksum, leading garbage, and back-to-back frame checks pass.

### Task 3: Stable reading and reference classification

**Files:**
- Create: `lib/GasMonitorCore/GasState.h`
- Create: `lib/GasMonitorCore/GasState.cpp`
- Create: `test/test_gas_state/test_main.cpp`

- [ ] **Step 1: Write the failing public-behavior test**

```cpp
GasState state;
state.observe({120, 79, 560}, 1000);
assert(state.snapshot(1500).sensorOnline);
assert(state.snapshot(1500).level == HchoLevel::Normal);
state.observe({140, 80, 570}, 2000);
assert(state.snapshot(2000).level == HchoLevel::Elevated);
assert(!state.snapshot(6001).sensorOnline);
```

- [ ] **Step 2: Implement EMA smoothing and the 80 µg/m³ reference line**

Use an EMA with `alpha=0.25`; mark the sensor offline after 3 seconds without a valid frame. Label the threshold as a reference to the GB/T 18883-2022 one-hour-average limit, not as a compliance verdict.

- [ ] **Step 3: Run the state test and verify GREEN**

Expected: online/offline, smoothing, and threshold boundary checks pass.

### Task 4: Serialize the dashboard status

**Files:**
- Create: `lib/GasMonitorCore/StatusJson.h`
- Create: `lib/GasMonitorCore/StatusJson.cpp`
- Create: `test/test_status_json/test_main.cpp`

- [ ] **Step 1: Write the failing JSON contract test**

```cpp
const auto json = makeStatusJson(snapshot);
assert(json.find("\"hchoUgM3\":80") != std::string::npos);
assert(json.find("\"eco2Estimated\":true") != std::string::npos);
```

- [ ] **Step 2: Implement bounded JSON output**

The response contains `sensorOnline`, `hchoUgM3`, `tvocUgM3`, `eco2Ppm`, `eco2Estimated`, `level`, `validFrames`, and `checksumErrors`.

- [ ] **Step 3: Run the JSON test and verify GREEN**

Expected: exact field values and JSON escaping pass.

### Task 5: Drive the SSD1306 OLED

**Files:**
- Create: `main/Ssd1306.h`
- Create: `main/Ssd1306.cpp`
- Create: `main/Font5x7.h`

- [ ] **Step 1: Implement the fixed hardware interface**

```cpp
bool begin(i2c_port_t port, std::uint8_t address = 0x3C);
void clear();
void drawText(int x, int page, const char* text, bool doubleSize = false);
void present();
```

Use 128×64 monochrome framebuffer, SSD1306 I2C control bytes `0x00` for commands and `0x40` for data, and address `0x3C`.

- [ ] **Step 2: Render deterministic pages**

OLED page 1 shows `HCHO`, the large µg/m³ number, `OK/HIGH`, and sensor state. Page 2 shows `TVOC`, estimated `eCO2`, valid frames, and checksum errors.

### Task 6: Integrate ESP32 UART, OLED, Wi-Fi, and HTTP

**Files:**
- Create: `main/CMakeLists.txt`
- Create: `main/main.cpp`
- Create: `main/web_page.h`

- [ ] **Step 1: Configure fixed pins and buses**

```cpp
constexpr gpio_num_t kGasRxPin = GPIO_NUM_5;
constexpr gpio_num_t kOledSdaPin = GPIO_NUM_15;
constexpr gpio_num_t kOledSclPin = GPIO_NUM_16;
constexpr int kGasBaud = 9600;
```

- [ ] **Step 2: Read and parse UART continuously**

Install UART1 RX on GPIO5 with 9600 8N1 and no flow control. Feed each received byte into `GasFrameParser`, update `GasState` for valid frames, and increment checksum errors for invalid frames.

- [ ] **Step 3: Refresh OLED and expose HTTP endpoints**

Refresh OLED at 4 Hz. Start AP `GasMonitor-XXXX` with password `gasmonitor`; expose `/`, `/api/status`, and `/api/health` at `192.168.4.1`.

- [ ] **Step 4: Build ESP32 firmware**

Run:

```powershell
idf.py set-target esp32s3
idf.py build
```

Expected: `build/gas_monitor.bin` is generated and partition size checks pass.

### Task 7: Wiring, limitations, and final verification

**Files:**
- Create: `README.md`
- Create: `docs/wiring.md`

- [ ] **Step 1: Document the exact wiring**

```text
TVOC-301 GND    -> ESP32 GND
TVOC-301 5V     -> ESP32 5V/VBUS
TVOC-301 SDA/TX -> ESP32 GPIO5
TVOC-301 AIO/SCL-> not connected in UART mode
OLED GND        -> ESP32 GND
OLED VCC        -> ESP32 3V3
OLED SCL        -> ESP32 GPIO16
OLED SDA        -> ESP32 GPIO15
```

- [ ] **Step 2: State the measurement boundary**

Document that this MEMS module is a trend/alert instrument, that `eCO2` is estimated rather than directly measured, and that an instantaneous 80 µg/m³ reading is not a one-hour GB/T 18883-2022 compliance measurement.

- [ ] **Step 3: Run final verification**

Run:

```powershell
cmake --build build-host
ctest --test-dir build-host --output-on-failure
idf.py build
git diff --check
```

Expected: all host tests pass, the ESP32-S3 firmware builds, and the diff has no whitespace errors.

