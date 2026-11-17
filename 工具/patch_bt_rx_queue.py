"""把 Arduino-ESP32 里 BluetoothSerial 的接收队列从 512 改到 20480。

为什么必须改
-----------
喵喵机 App 发一包 PRINT_DATA 最大 1920 字节（整帧 1930 字节）。
BluetoothSerial 默认 RX_QUEUE_SIZE=512，一帧都装不下，会静默丢包，
表现就是"App 显示发送成功，纸只打出一半或空白"。

用法
----
    python 工具/patch_bt_rx_queue.py

注意
----
1. 这是改 PlatformIO 的框架包，不在本仓库里；框架升级/重装后会被覆盖，
   升级过框架就再跑一次。
2. 队列是 1 字节一个元素，20480 大约占 20KB RAM，ESP32 上完全够用。
"""

import re
import sys
from pathlib import Path

NEW_SIZE = 20480


def find_framework() -> Path:
    home = Path.home()
    candidates = sorted((home / ".platformio" / "packages").glob("framework-arduinoespressif32*"))
    for c in candidates:
        f = c / "libraries" / "BluetoothSerial" / "src" / "BluetoothSerial.cpp"
        if f.exists():
            return f
    sys.exit("没找到 framework-arduinoespressif32 里的 BluetoothSerial.cpp，"
             "请确认 PlatformIO 已安装 espressif32 平台")


def main() -> int:
    path = find_framework()
    text = path.read_text(encoding="utf-8", errors="surrogateescape")

    m = re.search(r"#define\s+RX_QUEUE_SIZE\s+(\d+)", text)
    if not m:
        sys.exit(f"在 {path} 里没找到 RX_QUEUE_SIZE 定义")

    old = int(m.group(1))
    if old == NEW_SIZE:
        print(f"已经是 {NEW_SIZE}，无需修改：{path}")
        return 0

    text = text[:m.start()] + f"#define RX_QUEUE_SIZE         {NEW_SIZE}" + text[m.end():]
    path.write_text(text, encoding="utf-8", errors="surrogateescape")
    print(f"已修改 RX_QUEUE_SIZE: {old} -> {NEW_SIZE}")
    print(f"文件：{path}")
    print("接下来重新编译（pio run）才会生效。")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
