# OLED Hyperlegible Font Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the custom 5x7 OLED font with a compact Atkinson Hyperlegible bitmap font, render proper `µg/m³` and `eCO₂`, and give the HCHO reading clear primary hierarchy.

**Architecture:** Keep the existing SSD1306 I2C and framebuffer implementation. Add a host-testable UTF-8 bitmap-font module to `GasMonitorCore`; make `Ssd1306` render its glyphs; calculate all layout positions from real font metrics so values remain centered as their digit count changes. Check in only the small generated glyph subset needed by the product and attribute the SIL OFL source.

**Tech Stack:** C++17, ESP-IDF 5.5.3, SSD1306 128x64 monochrome framebuffer, Atkinson Hyperlegible Bold bitmap glyphs, CMake/CTest.

---

### Task 1: Lock the new font contract with a failing host test

**Files:**
- Create: `test/test_oled_font/test_main.cpp`
- Modify: `host/CMakeLists.txt`

- [x] **Step 1: Add a test executable that expects the new `OledFont` API**

The test must verify that `µ`, superscript `³`, and subscript `₂` resolve to real glyphs; the large `1` is wider than the legacy 10-pixel rendering; `2000` fits the 128-pixel primary region; and `5000` fits a 64-pixel secondary column.

- [x] **Step 2: Run the font test and verify RED**

Run:

```powershell
cmake -S host -B build-host -G Ninja
cmake --build build-host --target test_oled_font
```

Expected: compilation fails because `OledFont.h` does not exist yet.

### Task 2: Add the compact Atkinson bitmap-font module

**Files:**
- Create: `lib/GasMonitorCore/OledFont.h`
- Create: `lib/GasMonitorCore/OledFont.cpp`
- Create: `third_party/atkinson-hyperlegible/OFL.txt`
- Modify: `lib/GasMonitorCore/CMakeLists.txt`
- Modify: `host/CMakeLists.txt`

- [x] **Step 1: Define the host-testable font API**

Expose three faces—`Small`, `Medium`, and `Large`—plus UTF-8 decoding, glyph lookup, bitmap-pixel lookup, visible-top calculation, and text-width measurement. `Small` contains the UI labels and the `µ`, `³`, `₂` symbols; `Medium` contains secondary digits and the waiting-state letters; `Large` contains primary digits.

- [x] **Step 2: Embed only the required 1-bit glyphs**

Rasterize Atkinson Hyperlegible Bold at 10 px, 20 px, and 28 px, threshold to monochrome, and pack pixels row-major. Include the SIL Open Font License and provenance comment in the generated source.

- [x] **Step 3: Run the font test and verify GREEN**

Run:

```powershell
cmake --build build-host --target test_oled_font
ctest --test-dir build-host -R test_oled_font --output-on-failure
```

Expected: `test_oled_font` passes.

### Task 3: Render the new font through the existing SSD1306 framebuffer

**Files:**
- Modify: `main/Ssd1306.h`
- Modify: `main/Ssd1306.cpp`

- [x] **Step 1: Replace the scale argument with a font-face argument**

Change `drawText(int x, int y, const char* text, int scale)` to `drawText(int x, int y, const char* text, OledFont font)`, defaulting to `Small`.

- [x] **Step 2: Render UTF-8 glyph bitmaps**

Decode each code point, find the selected-face glyph, top-align the visible pixels of the full string, draw every set bitmap pixel into the existing framebuffer, and advance by the glyph's measured advance.

- [x] **Step 3: Build the ESP-IDF target**

Run:

```powershell
idf.py build
```

Expected: the firmware compiles with the new renderer and without references to `Font5x7.h`.

### Task 4: Rebuild the 128x64 information hierarchy from real font metrics

**Files:**
- Modify: `lib/GasMonitorCore/OledLayout.h`
- Modify: `lib/GasMonitorCore/OledLayout.cpp`
- Modify: `test/test_oled_layout/test_main.cpp`
- Modify: `main/main.cpp`

- [x] **Step 1: Update the layout test first**

Require the HCHO number to be centered using `Large`; center TVOC and eCO2 values independently inside two 64-pixel columns using `Medium`; and require the complete labels `TVOC µg/m³` and `eCO₂ ppm` to fit their columns.

- [x] **Step 2: Run the layout test and verify RED**

Run:

```powershell
cmake --build build-host --target test_oled_layout
ctest --test-dir build-host -R test_oled_layout --output-on-failure
```

Expected: the old `unitX` and scale-2 positioning assertions fail.

- [x] **Step 3: Implement metric-based positioning**

Use the real bitmap-font widths. Keep the primary region deliberately minimal: draw only the centered HCHO value with `Large` at y=0 and the centered `µg/m³` unit at y=20. Place the divider at y=31; draw `TVOC µg/m³` and `eCO₂ ppm` at y=34; and draw both lower values with `Medium` at y=47 so the labels' descenders end before the numeric row begins.

- [x] **Step 4: Run the layout test and verify GREEN**

Run:

```powershell
cmake --build build-host --target test_oled_layout
ctest --test-dir build-host -R test_oled_layout --output-on-failure
```

Expected: `test_oled_layout` passes for one-digit and four-digit HCHO values.

### Task 5: Full verification

**Files:**
- Modify if needed: `README.md`

- [x] **Step 1: Run all host tests**

```powershell
cmake -S host -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

Expected: all host tests pass with zero failures.

- [x] **Step 2: Run a clean firmware build**

```powershell
idf.py build
```

Expected: ESP-IDF build exits with code 0 and reports the generated firmware sizes.

- [x] **Step 3: Inspect the final source references**

```powershell
rg -n "Font5x7|UG/M3|ECO2 EST|drawText\(.+,\s*[23]\)" main lib test
```

Expected: no active renderer or UI call uses the legacy font, ASCII-only unit, old eCO2 label, or numeric scale argument.

No commit step is included because this workspace is not a Git repository.
