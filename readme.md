# 智能硬件系统课程设计 · iot_project

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![课程设计报告](https://img.shields.io/badge/%E8%AF%BE%E8%AE%BE%E6%8A%A5%E5%91%8A-Markdown-blue.svg)](docs/智能硬件系统课设报告.md)

一套跑在真实硬件上的物联网系统：实验箱上的嵌入式 C 程序采集传感器数据、执行控制指令，Ubuntu 虚拟机做 MQTT broker，Windows 上的 Qt 程序做图形化控制端。

> **📄 课程设计报告（9 章，含环境搭建、代码实现、实验效果与总结）**
> **[`docs/智能硬件系统课设报告.md`](docs/智能硬件系统课设报告.md)**

## 三个角色

| 角色 | 跑在哪 | 干什么 |
| --- | --- | --- |
| 设备端 `mqtt_iot` | 实验箱 FS_EMB6818（S5P6818 ARM + 嵌入式 Linux） | 每 5 s 读一次 DHT11 并发布；订阅控制主题，把命令分发给各外设 |
| Broker | Ubuntu 虚拟机（Mosquitto） | 消息中转站，设备端和控制端都连它 |
| 控制端 `Controller` | Windows（Qt 4.8 GUI） | 温湿度实时显示 + 外设控制按钮 + 实验箱面板仿真预览 |

三者之间只走标准 MQTT，两个方向的数据流：

- **上行**：实验箱 → `s5p6818/env/data` → broker → 控制端，负载是 JSON：`{"hum":N,"temperature":N}`
- **下行**：控制端 → `s5p6818/device/ctrl` → broker → 实验箱，负载是命令字符串（见下方[命令速查](#命令速查)）

## 效果

### 控制端界面

![Controller 控制端界面](docs/images/controller-gui.png)

左栏是 MQTT 连接设置、温湿度实时读数和各外设的控制按钮，右栏是实验箱面板的仿真视图 —— 点左边的按钮，右边的 LCD、LED 会跟着变。

### 实物接线

![实验箱实物与虚拟机接线](docs/images/labbox-and-vm.png)

实验箱通过网线和串口接实验室电脑，文件系统由虚拟机的 NFS 共享。

### 系统架构

![系统总体架构图](docs/images/architecture.png)

设备端跑在实验箱的嵌入式 Linux 上，通过网线和 NFS 共享根文件系统；broker 跑在 Ubuntu 虚拟机里；控制端是 Windows 上的 Qt 程序。虚拟机同时充当实验箱的根文件系统服务器，实验箱还通过串口接实验室电脑，用 PuTTY 看启动日志。

### 软件设计与数据流

![软件设计](docs/images/software-design.png)

设备端软件内部结构：`on_ctrl_message()` 把收到的 MQTT 命令分发给各外设驱动，DHT11 采集线程把温湿度发布出去。

![数据流](docs/images/dataflow.png)

## 快速开始

### 1. 设备端：交叉编译并部署（Ubuntu 虚拟机上执行）

```shell
chmod +x build_and_deploy.sh
./build_and_deploy.sh
#   交叉编译器：/usr/local/toolchain/toolchain-4.5.1
#   部署目标：/source/rootfs/root/mqtt_iot/（实验箱 uboot 用 NFS 挂载这个根）
#   RPATH 设为 $ORIGIN，libmosquitto.so.0 和主程序放同一个目录即可
```

> 依赖 ARM 版 libmosquitto（`install/` 里已预编译，也可用 `build_libmosq.sh` 自己编）。

### 2. broker：虚拟机上起 Mosquitto

```shell
mosquitto -d          # 默认 1883 端口
```

broker 地址默认写在 [`src/config.h`](src/config.h) 的 `MQTT_HOST`，也可以用命令行参数覆盖：

```shell
/source/rootfs/root/mqtt_iot/mqtt_iot 192.168.100.192
```

### 3. 控制端：编译并运行（Windows 上执行）

```powershell
qmake Controller.pro
mingw32-make
```

更详细的说明和端到端自测脚本见 [`Controller/README.md`](Controller/README.md)。

> ⚠️ 实验箱的 Linux 根是 NFS 挂载的，没有 devtmpfs，所以 `/dev` 是空目录。
> `mqtt_iot` 启动时会自己 `mknod /dev/dht11 c 804 0` 建节点，不需要手动干预。

## 命令速查

服务跑起来之后，不用写代码就能测 —— 直接往控制主题发字符串：

```shell
mosquitto_sub -h 192.168.100.192 -t 's5p6818/#' -v                              # 监听全部话题
mosquitto_pub -h 192.168.100.192 -t s5p6818/device/ctrl -m "led_on"              # 点亮全部 LED
```

| 外设 | 命令字符串 |
| --- | --- |
| LED | `led_on`、`led_off`（全开/全关）、`led1_on` ~ `led4_on`、`led1_off` ~ `led4_off` |
| 舵机 | `servo_0`、`servo_45`、`servo_90`、`servo_135`、`servo_180` |
| LCD | `lcd_red`、`lcd_green`、`lcd_blue`、`lcd_white`、`lcd_black`、`lcd_yellow` |
| 蜂鸣器 | `buzzer_on` / `beep_on`（实验箱蜂鸣器驱动未安装，应用层只记日志，不会崩） |

命令字符串严格对应 [`src/main.c`](src/main.c) 的 `on_ctrl_message()`，控制端按钮发的也是同一套字符串。

## 实验环境

| 设备 | 型号 | 在系统里的角色 |
| --- | --- | --- |
| 实验箱（下位机） | 华清远见 FS_EMB6818，8 核 Cortex-A53 | 设备端 `mqtt_iot` 的运行平台 |
| 宿主机（上位机） | Windows 10 + VMware | 跑控制端，接串口和网线 |
| 虚拟机 | Ubuntu 12.04 64-bit | 跑 broker、NFS、设备端交叉工具链 |

VMware 有两种网络模式，用途不同：

- **NAT**（`192.168.127.x`）：虚拟机走无线网卡上网、装包，也能 `ssh linux@192.168.127.100` 进去
- **桥接**（`192.168.100.x`）：虚拟机接到有线网卡上，**和实验箱同一网段**，这样才能通信

实验箱通过网线接宿主机有线网卡、通过串口接宿主机，波特率 115200，串口号以设备管理器为准。

**桥接模式下把虚拟机 IP 定死**（改 `/etc/network/interfaces` 后重启网络）：

```
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet static
address 192.168.100.192
netmask 255.255.255.0
network 192.168.100.0
```

**实验箱从 NFS 启动**（在实验箱 uboot 里设置，与实验箱同网段即可）：

```uboot
setenv bootcmd ext4load mmc 2:1 0x48000000 uImage\;bootm 0x48000000
setenv bootargs root=/dev/nfs nfsroot=192.168.100.192:/source/rootfs,proto=tcp console=ttySAC0,115200 init=/linuxrc ip=192.168.100.111:::::eth0:off
```

## 目录结构

```
iot_project/
├── src/                  设备端 ARM C 程序（mqtt_iot）
│   ├── main.c            入口：信号处理 + MQTT + DHT11 采集线程 + 命令分发
│   ├── mqtt_client.c/h   libmosquitto 0.15 封装
│   ├── dht11.c/h         DHT11 读取（自动 mknod 建 /dev/dht11）
│   ├── led.c/h           sysfs 控制 LED
│   ├── lcd.c/h           framebuffer 填色
│   ├── servo.c/h         舵机 ioctl
│   ├── beep.c/h          蜂鸣器 ioctl
│   ├── logger.h          简易日志
│   ├── config.h          broker 地址、topic 名等集中配置
│   └── Makefile          arm-linux-gcc 交叉编译
├── Controller/           控制端 Qt 4.8 GUI（qmake 工程）
├── install/              预编译的 ARM libmosquitto.so.0 + mosquitto.h
├── docs/                 课程设计报告 + 任务书 + 参考材料
├── build_and_deploy.sh   设备端：交叉编译 + 部署到实验箱 NFS 根文件系统
├── build_libmosq.sh      交叉编译 libmosquitto 0.15 到 install/
└── LICENSE
```

## 相关文档

- 课程设计报告：[`docs/智能硬件系统课设报告.md`](docs/智能硬件系统课设报告.md)（9 章，配图在 `docs/images/report/`）
- 老师给的任务书：[`docs/任务书.MD`](docs/任务书.MD)
- 参考材料（老师给的课件与 MQTT 应用参考）：[`docs/参考资料/`](docs/参考资料/)
- 控制端说明与自测脚本：[`Controller/README.md`](Controller/README.md)

## 许可协议

[MIT](LICENSE) © 2026 JERRY (LZJNano)