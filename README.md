# ESP32 蓝牙热敏打印机 (ESP32_BT_printer)

基于 ESP32-WROOM-32E + JX-2R-01 打印头（384 点 / 48mm 可打印宽）的便携热敏打印机，
VS Code + PlatformIO + Arduino 框架。

## 现在能做什么

- [x] 按键打印测试图（200 行位图，开机即可用）
- [x] 打印头驱动：SPI 送数、LAY 锁存、6 段 STB 分段加热、VH 升压使能
- [x] 步进电机走纸（八拍，32 步/mm）
- [x] 缺纸检测（GPIO35，2 秒一次非阻塞告警）
- [x] 电池电压 / 电量查表、打印头 NTC 测温与过温保护
- [x] 经典蓝牙 SPP，设备名 `MaoPaperang_Q`
- [x] 喵喵机 App 协议：帧解析 + CRC32 + ACK + **握手 10 连** + **打印数据通路**
- [ ] 打印任务与接收解耦（双任务 + 环形缓冲）
- [ ] 设备主动状态上报（缺纸/完成/过温）

## 上位机

一台打印机，手机和电脑都能当上位机，**同一时刻只能连一个**：

| 上位机 | 连接方式 | 说明 |
|---|---|---|
| 安卓手机 | 经典蓝牙 SPP | 系统蓝牙里配对 `MaoPaperang_Q`，再装喵喵机 App 或自制 App |
| Windows 电脑 | 经典蓝牙 SPP → COM 口 | 配对后在"设备和打印机 → 属性 → 服务"里勾选串行端口，用"传出"COM |
| iPhone | ❌ 用不了 | iOS 第三方 App 只开放 BLE，需改固件 |

调试最快的方式：串口蓝牙助手发 HEX。发 `02 04 00 00 00 2A 33 80 33 03`（GET_VERSION），
应收到 `02 04 00 01 00 00 5F CC 48 08 03`。

## 通信协议

```
0x02 | type | idx | len(2,LE) | payload | CRC32(4,LE) | 0x03
```

- CRC32 标准 zlib 算法；
- 数据帧 CRC 覆盖 payload；ACK 帧覆盖前 6 字节；空载荷帧两种写法都放行；
- 查询类指令回 `SENT_*` 数据帧，其余回 11 字节 ACK；
- 握手顺序、打印流程、指令表见 [`文档/通用打印方案设计.md`](文档/通用打印方案设计.md)
  与 [参考文档](文档/打印问题诊断总结.md)。

## 编译与烧录

```bash
pip install platformio          # 或直接用 VS Code PlatformIO 插件
python 工具/patch_bt_rx_queue.py # 必须：把蓝牙接收队列 512 改成 20480，否则大图丢包
pio run                         # 编译
pio run -t upload               # 烧录（默认 COM 口，可按需改 platformio.ini）
pio device monitor              # 串口日志 115200
```

> 框架升级后 `工具/patch_bt_rx_queue.py` 需要重新执行（它改的是 PlatformIO 包里的文件）。

## 注意

- **速度**：单段加热 3.5ms × 6 段串行，一行约 34ms ≈ 3.7mm/s，A4 就别想了，
  30mm 以内的小票/便签才舒服。发热、升压回充、电池掉压是三条硬约束。
- **分区**：当前 `default.csv`，Flash 已用 82%，加中文字库（GB2312 约 211KB）需换 `huge_app.csv`。
- **保护**：缺纸绝不加热；过温（≥80℃）中止；打印中铁律见 [`文档/笔记.md`](文档/笔记.md)。
