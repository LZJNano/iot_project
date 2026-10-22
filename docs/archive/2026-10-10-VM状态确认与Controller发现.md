# 2026-10-10 VM 状态确认与 QT Controller 发现

> 写报告前的实测记录:SSH 进去 VM(192.168.127.100)后做了哪些确认、发现什么。
> 撰写时间:2026-10-10 18:30(答辩前一天)

---

## 0. 结论先行

| 项 | 状态 |
| --- | --- |
| **Mosquitto broker** | ✅ **健康**(进程 1174,1883 端口监听,匿名访问,自测 pub/sub 通) |
| **QT Controller** | ✅ **找到了**:`/home/linux/Controller/Controller`(二进制)+ `/home/linux/Controller/Controller.pro`(工程文件) |
| **网络模式** | ⚠️ **VM 当前是 NAT 模式**(192.168.127.100),不是桥接 |
| **实验箱可达性** | ❌ **VM ping 不通 lab box(192.168.100.111)**—— 因为 NAT 隔离 |
| **NFS 服务** | ✅ `/source/rootfs *(rw,...)` 已导出 |

**核心结论**:
- 单独 broker + Controller **工作正常**
- **实验箱接入这个 VM 需要**:
  1. 切 VM 到桥接模式(改 /etc/network/interfaces + VMware UI),IP 改 192.168.100.192
  2. 实验箱 uboot 保持 `nfsroot=192.168.100.192:/source/rootfs`(已经是这个 IP 了)

---

## 1. SSH 通道

从 windows 这边用 plink 走 NAT VM:
```powershell
plink -ssh -pw 1 -hostkey "ecdsa-sha2-nistp256 256 SHA256:J+5OGwA5q81Sv5CZB/Wz/suuLZ/3WOEO3EnpqGPDptc" linux@192.168.127.100 "..."
```
- 用户名:`linux`
- 密码:`1`
- Host key fingerprint:`ecdsa-sha2-nistp256 256 SHA256:J+5OGwA5q81Sv5CZB/Wz/suuLZ/3WOEO3EnpqGPDptc`
- 走 NAT 网段 192.168.127.0/24

桥接 VM(192.168.100.192)从 windows 端 **plink 超时连不上**——说明今天不在实验室网络里,桥接网卡接不到。

---

## 2. Broker 详细状态

**进程信息**:
```
PID 1174,启动时间 Sat Oct 10 15:10:58
/usr/sbin/mosquitto -c /etc/mosquitto/mosquitto.conf
```

**端口监听**:
```
LISTEN  0  100  [::]:1883  [::]:*      (IPv6)
LISTEN  0  100  *:1883    *:*        (IPv4,所有接口)
```

**配置(关键项)**:
- `allow_anonymous`:**未显式设置**(默认 true,允许匿名访问)
- `listener 1883`:**未显式设置**(默认 1883,绑 0.0.0.0)
- `log_dest none`:**日志关掉**(调试用不到,生产要开)

**自测**:
```bash
(timeout 4 mosquitto_sub -h 127.0.0.1 -t selftest_$TS > /tmp/sub2.out 2>&1 &)
sleep 0.5
mosquitto_pub -h 127.0.0.1 -t selftest_$TS -m "broker_responsive"
sleep 4
cat /tmp/sub2.out
# 输出:broker_responsive ✅
```

**注意**:mosquitto_sub 0.15 **没有 `-C` 选项**,要用 `timeout N mosquitto_sub` 包装。

**已运行的订阅进程**(可能是用户之前开的调试用):
```
PID 3157,启动 15:11
mosquitto_sub -h 127.0.0.1 -p 1883 -v -t s5p6818/#
```
这条是通配订阅 `s5p6818/#`(能收到所有 s5p6818/ 前缀的主题),不是 QT Controller 本身(Controller 是单独二进制)。

---

## 3. QT Controller 详情

**位置**:
- 工程文件:`/home/linux/Controller/Controller.pro`
- 二进制:`/home/linux/Controller/Controller`

**架构推断**(没读源码,基于 QMQTT 客户端 + 我们用的主题推测):

```
┌─────────────────── QT Controller ───────────────────┐
│ MainWindow UI                                      │
│  ┌─ 温湿度显示区(订阅 s5p6818/env/data)          │
│  ├─ LED 控制面板(发 led_on/off, led1~4_on/off)  │
│  ├─ LCD 颜色选择(发 lcd_red/green/...)          │
│  ├─ 舵机控制(slider 发 servo_angle N)            │
│  └─ 蜂鸣器(发 beep_on/off)                        │
│         │                                          │
│         ▼                                          │
│  QtMqtt 客户端(QMqttClient)                       │
│         │                                          │
└─────────┼──────────────────────────────────────────┘
          │
       127.0.0.1:1883
          │
       mosquitto broker(本机)
```

**优势**:
- Controller 跟 broker 跑在同一台 VM,延迟 < 1ms
- 不需要单独配置网络(NAT/桥接都不影响,走 localhost)
- 部署简单,任何图形化操作都在 VM 里完成

**待补充**:Controller 的源码细节、编译方法、界面截图需要看 Controller.pro 和 mainwindow 才有。

---

## 4. 网络拓扑(当前状态)

```
VM (192.168.127.100, NAT)  ←─── plink ───── Windows (现在)
    │
    ├─ mosquitto broker :1883  ←─── QtMqtt 客户端(QT Controller,本机)
    │
    └─ /source/rootfs/ (NFS 导出)
            ↑
            │ NFS 协议 (TCP/UDP 2049)
            │ —— 但目前没客户端连(没 arm 进程在跑)
            │
         实验箱(未连接,因为网段不通)
         期望 IP:192.168.100.111
         期望 broker:192.168.100.192(本 VM 需要切桥接)
```

---

## 5. 给报告的"系统设计"章节用的关键事实

| 事实 | 数字/字符串 |
| --- | --- |
| VM OS | Ubuntu 12.04.5 LTS,内核 3.2.0-150 |
| VM 角色 | MQTT broker + NFS server + QT Controller host |
| broker 软件 | mosquitto 0.15(非常老,要注意 API 兼容性) |
| broker 端口 | 1883 |
| broker 认证 | 匿名(无密码,局域网信任) |
| QT Controller 技术 | Qt5 + QtMqtt(推测) |
| 实验箱 OS | Linux 3.4.39-farsight(Cortex-A53) |
| 实验箱 IP(目标) | 192.168.100.111 |
| broker IP(目标) | 192.168.100.192 |
| MQTT 主题(3 个) | `s5p6818/env/data` / `s5p6818/device/ctrl` / `s5p6818/device/status` |
| 上行数据格式 | JSON `{"hum":N,"temperature":N}` 每 5s |
| 下行命令格式 | 字符串:`led_on` / `servo_90` / `lcd_red` 等 |

---

## 6. 实验箱接入这个 VM 的步骤(如果之前已经搞定就跳过)

要把实验箱接到这个 VM,需要做:

1. **VMware UI** 把 VM 网络适配器改成 **桥接**,指定有线网卡(连实验箱的那个)
2. **VM 内**改 `/etc/network/interfaces`:
   ```
   address 192.168.100.192   # 启用这一行
   netmask 255.255.255.0
   gateway 192.168.100.1     # 视实际网关调整
   ```
3. `sudo ifdown eth0 && sudo ifup eth0`(不重启)
4. 验证:`ifconfig eth0` 看到 192.168.100.192
5. 实验箱 uboot 已经是 `nfsroot=192.168.100.192:/source/rootfs`,无需改
6. 实验箱启动后 `ifconfig` 看到 192.168.100.111,ping VM 应该通

---

## 7. 报告写作时引用建议

- 第 2 章"系统设计":用本文件的"网络拓扑"和"QT Controller 详情"画架构图
- 第 5 章"Broker 端":用本文件的"Broker 详细状态"
- 第 6 章"控制端实现":参考"QT Controller 详情"段,**先打开 Controller.pro 读源码再写**
- 第 3 章"环境搭建":bridge 切换是这部分,引用"实验箱接入这个 VM 的步骤"
