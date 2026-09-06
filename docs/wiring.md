# Gas Monitor 接线与上电检查

交付基线：2026-09-01 21:52:17 构建的 `gas_monitor.bin`，同日晚成功烧录日志验证了 870,768 bytes 的应用镜像。OLED 使用更新后的 **4-wire SPI** 六针版本，本说明已纠正旧 I²C 接线。

## 最终引脚表

```text
TVOC-301                         ESP32-S3
GND       --------------------  GND
5V        --------------------  5V / VBUS
SDA/TX    --------------------  GPIO5 (UART1 RX)
AIO/SCL   --------------------  不接

SSD1306 OLED                     ESP32-S3
GND       --------------------  GND
VCC       --------------------  3V3
SCL       --------------------  GPIO12 (SPI SCLK)
SDA       --------------------  GPIO11 (SPI MOSI)
RES       --------------------  GPIO10 (Reset)
DC        --------------------  GPIO9 (Data/Command)
```

## 上电前检查

1. 所有模块必须共地。
2. OLED `VCC` 必须接 `3V3`，不能接传感器的 `5V`。
3. TVOC-301 使用照片丝印的 `SDA/TX` 输出；`AIO/SCL` 在 UART 模式不接。
4. 首次连接 GPIO5 前，确认传感器 TX 为兼容的 3.3 V UART 电平；5 V 输出或电平未知时先使用合适的单向电平转换，不能把“低于极限电压”直接当成已验证安全。
5. 传感器进气孔朝向开放空气，不能用胶带、热缩管或外壳挡住。
6. OLED 的 SCL/SDA 是 SPI 时钟/数据，不是 I²C；RES/DC 必须连接。本固件没有软件 CS 引脚，六针模块须内部已使能 CS；若是七针模块，应根据模块资料将低有效 CS 固定为有效，不能悬空。

## 正常启动现象

```text
TVOC-301 UART ready: RX=GPIO5 baud=9600
SSD1306 SPI ready: MOSI=GPIO11 SCLK=GPIO12 RES=GPIO10 DC=GPIO9
Wi-Fi ready: SSID=GasMonitor-XXXX password=gasmonitor
Dashboard: http://192.168.4.1
TVOC=... HCHO=... eCO2=...
```

如果 OLED 显示 `SENSOR OFFLINE`，但 Wi-Fi 页面能打开，说明 ESP32 固件正常、UART 没收到合法帧。依次检查传感器 5V、共地、TX→GPIO5、9600 8N1 和 checksum error。

## 串口未识别时

开发板亮灯只证明 USB 有 5V，不证明数据线可用。串口号以接收者电脑当前枚举为准，旧机器的一次枚举结果不是当前状态。

按以下顺序处理：

1. 把线换到开发板另一个 USB-C 口。
2. 更换一根已确认能传文件/烧录的 USB 数据线。
3. 按住 `BOOT`，点按一次 `RST`，松开 `BOOT`，重新检查设备管理器。
4. 成功时应出现新的 `COMx` 或 `USB/JTAG/Serial Debug Unit`；不要选择蓝牙 COM25/COM26。

## 使用与测量边界

连接热点 `GasMonitor-XXXX`（默认演示密码 `gasmonitor`），打开 `http://192.168.4.1`。OLED 甲醛主读数使用 28px 数字字体，下方显示 TVOC 与估算 eCO₂。默认热点密码公开已知，不作为私密网络使用。

该 MEMS 模块用于趋势观察，不是专业空气检测或生命安全报警器；eCO₂ 不是 NDIR CO₂ 直接测量。瞬时显示和提醒线不能用来判定室内空气达标或安全。
