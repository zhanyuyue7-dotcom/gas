# Gas Monitor

ESP32-S3 室内气体趋势监测器。通过 UART 读取 TVOC-301 三合一 MEMS 模块，在 0.96 英寸 SSD1306 OLED 和手机网页上显示甲醛、TVOC 与估算 eCO₂。

## 硬件

- ESP32-S3 双 USB-C 开发板
- TVOC-301 / TVOC-CH₂O-CO₂ MEMS 模块
- 0.96 英寸 SSD1306 OLED：128×64、4-wire SPI

## 接线

| 模块引脚 | ESP32-S3 | 说明 |
|---|---|---|
| TVOC-301 `GND` | `GND` | 必须共地 |
| TVOC-301 `5V` | `5V/VBUS` | 传感器电源 |
| TVOC-301 `SDA/TX` | `GPIO5` | UART TX → ESP32 RX |
| TVOC-301 `AIO/SCL` | 不接 | 当前固件使用 UART |
| OLED `GND` | `GND` | 共地 |
| OLED `VCC` | `3V3` | 禁止接 5V |
| OLED `SCL` | `GPIO12` | SPI clock |
| OLED `SDA` | `GPIO11` | SPI MOSI |
| OLED `RES` | `GPIO10` | Reset |
| OLED `DC` | `GPIO9` | Data/Command |

不要只按 4Pin 插座的左右顺序接线，以模块正面丝印 `GND / 5V / AIO-SCL / SDA-TX` 为准。传感器图片只明确标注 5V 供电，没有明确给出 TX 高电平规格；首次直连前应测量 `SDA/TX→GND` 空闲高电平，超过 `3.6V` 时必须增加 5V→3.3V level shifter。

完整说明见 [docs/wiring.md](docs/wiring.md)。

## 运行方式

1. 上电后 OLED 顶部仅显示甲醛主读数及 `µg/m³`，下方显示 TVOC 和估算 `eCO₂`。
2. 手机连接热点 `GasMonitor-XXXX`，密码 `gasmonitor`。
3. 浏览器打开 `http://192.168.4.1`。
4. API：`GET /api/status`；健康检查：`GET /api/health`。

## 协议

UART：`9600 baud / 8 data bits / 1 stop bit / no parity / no flow control`。

```text
Byte 0   0x2C
Byte 1   0xE4
Byte 2-3 TVOC，big-endian，μg/m³
Byte 4-5 HCHO，big-endian，μg/m³
Byte 6-7 eCO₂，big-endian，ppm
Byte 8   Byte 0-7 累加和的低 8 位
```

解析器会跳过前导垃圾、校验 checksum，并在 3 秒没有有效帧时把传感器标记为离线。显示值使用 `alpha=0.25` EMA，降低 MEMS 读数跳动。

## 构建与烧录

在 ESP-IDF 5.5.3 PowerShell 中运行：

```powershell
Set-Location 'D:\Downloads\contentcreater-resources\gas'
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

当前固件产物：`build/gas_monitor.bin`。

## OLED 字体

OLED 使用从 Atkinson Hyperlegible Bold 栅格化的紧凑 1-bit bitmap
字形：甲醛主读数为 28 px，TVOC/eCO₂ 次读数为 20 px，标签为 10 px；
固件原生显示 `µg/m³`、`eCO₂` 和 `ppm`。运行时不依赖外部字体库，
只编入界面实际需要的字形子集。

字体来源：[googlefonts/atkinson-hyperlegible](https://github.com/googlefonts/atkinson-hyperlegible)；
授权协议见 [`third_party/atkinson-hyperlegible/OFL.txt`](third_party/atkinson-hyperlegible/OFL.txt)。

Host tests：

前置：CMake、Ninja、C++17 compiler 和 Node.js。

```powershell
cmake -S host -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

## 测量边界

- 本模块是 MEMS 趋势传感器，不是法定计量或专业室内空气检测设备。
- 模块的 `eCO₂` 是依据气敏响应估算的等效值，不是 NDIR CO₂ 直接测量。
- 固件中的 `80 μg/m³` 只是一条提醒参考线。现行 [GB/T 18883-2022《室内空气质量标准》](https://openstd.samr.gov.cn/bzgk/std/newGbInfo?hcno=6188E23AE55E8F557043401FC2EDC436)要求甲醛 `≤0.08 mg/m³`，备注为 **1 小时平均**；OLED/Web 显示的是瞬时 EMA，不能据此宣称房间合格或不合格。
- 本次交付使用六针 SPI OLED；旧 HiSpark I²C 显示板资料不适用于当前固件引脚。

## GitHub CI 与交付 ZIP

`.github/workflows/ci.yml` 在 push / PR 时执行 Python 打包回归测试、C++/Web/网页烧录包 host tests 和 ESP-IDF 5.5.3 的 ESP32-S3 固件构建，全部通过后上传 `firmware-delivery` artifact（保留 90 天）。未配置 GitHub Pages 自动部署。

交付固定为 `delivery.json` 锁定的最新已烧录 SPI BIN（2026-09-01 21:52），不是旧网页烧录包里的 I²C 版本。`docs/firmware`、`docs/checksums.json`、接线手册及网页接线已同步到该版本。CI 编译验证源码，但不替换已烧录快照；本次交付没有重新接板实测。交付标识独立于原网页的 0.1.0 版本号。

```powershell
python -m unittest discover -s scripts -p 'test_*.py' -v
python scripts/package_delivery.py
```

生成 `dist/gas-delivery-20260906.zip`，仅含 `接线手册.md`、`bootloader.bin`、`partition-table.bin`、`gas_monitor.bin`。手册包含地址、校验值、使用边界和字体 OFL 许可证。ZIP 不含源码、网页安装器、日志、截图或本机配置。

后续改固件需先确认实测版本与接线，再更新 `delivery.json` 和网页包的校验值。旧本地 `dist/Gas-Monitor-ESP32-v0.1.0.zip` 保留供追溯，不属于本次交付。
