# Controller - IoT 课设控制端 (Qt 4.8)

Qt 4.8 写的 IoT 控制端 GUI。开发机 Windows + Qt 4.8.7(`D:\QT`),
目标平台 Ubuntu 12.04 + Qt 4.8(源码兼容,`qmake && make` 即可)。

## 文件

| 文件 | 作用 |
| --- | --- |
| `Controller.pro`         | qmake 工程文件 |
| `main.cpp`               | 入口 |
| `MainWindow.h/.cpp`      | 主窗口(连接/状态/LED/舵机/蜂鸣器/LCD/日志) |
| `MqttProcess.h/.cpp`     | QProcess 包装 `mosquitto_pub`/`mosquitto_sub`,不依赖 libmosquitto |
| `selftest.sh`            | VM 上端到端自测脚本(启动 mosquitto_sub + xvfb-run Controller + mosquitto_pub 模拟) |

## 设计

- **不依赖额外的 C 客户端库**:用 `QProcess` 调 `mosquitto_pub`/`mosquitto_sub`。
  Ubuntu 端 `apt install mosquitto-clients`,Windows 端单独装 mosquitto。
- **启动后自动连接**:`QTimer::singleShot(300, this, onConnectClicked)`,
  打开就能用,不必手动点连接。host/base topic 在 UI 里可改。
- **topic 约定对齐下位机 `src/config.h`**:
  - 上行(订阅): `<base>/env/data` ← JSON `{"hum":N,"temperature":N}`
  - 下行(发布): `<base>/device/ctrl` → 命令字符串
- **命令字符串严格对齐 `src/main.c::on_ctrl_message`**:
  - `led_on` / `led_off` (全开/全关)
  - `led1_on` ~ `led4_on` / `..._off` (单 LED,FS_EMB6818 只有 4 个)
  - `buzzer_on` / `beep_on` (蜂鸣器)
  - `servo_0` / `servo_45` / `servo_90` / `servo_135` / `servo_180`
  - `lcd_red` / `lcd_green` / `lcd_blue` / `lcd_white` / `lcd_black` / `lcd_yellow`

## 默认配置

- **host**: `127.0.0.1` (本机,VM 自测方便)
- **port**: `1883`
- **base topic**: `s5p6818`
- 答辩时改 host = `192.168.100.192`(桥接模式 VM IP)

## Windows 编译

```powershell
$env:Path = "D:\QT\bin;D:\QT\mingw32\bin;" + $env:Path
cd D:\projects\iot_project\Controller
qmake Controller.pro
mingw32-make
# 产物:debug/Controller.exe (765KB)
```

## Ubuntu 12.04 编译

```bash
sudo apt-get install -y qt4-dev-tools build-essential mosquitto-clients xvfb
cd ~/Controller
qmake Controller.pro
make -j4
# 产物:./Controller (71KB,x86_64)
```

## 端到端自测(已验证通过)

```bash
cd ~/Controller
bash selftest.sh
# 看 /tmp/ctrl.log (mosquitto_sub 收到 device/ctrl)
# 看 /tmp/ctrlr.log (Controller stderr,过滤 Fontconfig 行)
```

预期输出:
- `/tmp/ctrl.log`: `s5p6818/device/ctrl led1_on` 等 3 条
- `/tmp/ctrlr.log`: `[HH:MM:SS] RX env/data = {...}`、`RX device/ctrl = ...`

## 远程 VM 上操作(Windows → VM via plink)

```powershell
# 一次性安装
. C:\Users\JERRY\AppData\Local\Temp\vm.ps1
vm "echo 1 | sudo -S apt-get install -y qt4-dev-tools mosquitto-clients xvfb"

# 推代码
$PK="ecdsa-sha2-nistp256 256 SHA256:J+5OGwA5q81Sv5CZB/Wz/suuLZ/3WOEO3EnpqGPDptc"
pscp -pw 1 -hostkey $PK *.cpp *.h *.pro *.sh linux@192.168.127.100:/home/linux/Controller/

# 编译
vm "cd ~/Controller && qmake Controller.pro && make -j4"

# 跑(桌面 VM 直接 ./Controller;无桌面用 xvfb-run)
vm "xvfb-run -a ~/Controller/Controller &"

# 自测
vm "bash ~/Controller/selftest.sh"
```

## 已知问题

- stderr 中文乱码(终端 locale 是 POSIX),GUI 内 QTextEdit 显示正常
- VM 重复运行 selftest 时 `mosquitto_sub` 偶发看到上一次的输出(旧 log 没清),
  selftest.sh 第一次跑会 `cat /tmp/ctrl.log` 输出双份 — 加 `: > /tmp/ctrl.log` 已修
- 实验箱没开时下位机命令链不能联调,只能本地 broker 自测
- Controller 是 x86_64 二进制,如果要部署到 ARM 实验箱(Qt4 桌面),需重交叉编译