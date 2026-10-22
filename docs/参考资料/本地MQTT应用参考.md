# 1-本地MQTT应用参考

## 3.1 Ubuntu搭建Mosquitto MQTT服务器

Ubuntu虚拟机与主机通过有线网卡桥接，用于资源下载，配置如下：

1. **主机：无线局域网适配器WLAN**
   - 连接特定的DNS后缀
   - IPv6地址：`2001:250:21b:8000::3509`
   - 本地链接IPv6地址：`fe80::3485:a673:9b10:40e8%10`
   - IPv4地址：`10.153.154.59`
   - 子网掩码：`255.255.0.0`
   - 默认网关：`fe80::5e64:7aff:feca:4207%10`、`10.153.0.1`

2. **虚拟机：**

```text
auto eth0
iface eth0 inet static
address 10.153.143.200
netmask 255.255.0.0
gateway 10.153.0.1
network 10.153.143.0
broadcast 10.153.143.255
dns-nameservers 8.8.8.8 114.114.114.114
```

### 3.1.1 安装服务

执行终端命令更新并安装Mosquitto服务与客户端工具：

```bash
sudo apt-get update
sudo apt-get install mosquitto mosquitto-clients -y
```

### 3.1.2 配置服务器（局域网可访问）

1. 打开配置文件：

```bash
sudo vim /etc/mosquitto/mosquitto.conf
```

2. 文件末尾添加以下配置（关闭权限验证、开放全网监听）：

```text
allow_anonymous true
listener 1883 0.0.0.0
```

### 3.1.3 重启服务并设置开机自启

```bash
sudo systemctl restart mosquitto
sudo systemctl enable mosquitto
sudo systemctl status mosquitto
```

### 3.1.4 防火墙放行端口

```bash
sudo ufw allow 1883
```

### 3.1.5 测试服务器可用性

终端1（订阅）：

```bash
mosquitto_sub -h 本机UbuntuIP -p 1883 -t test
```

终端2（发布）：

```bash
mosquitto_pub -h 本机UbuntuIP -p 1883 -t test -m "mqtt serverok"
```

收到消息即服务器搭建成功。

### 3.1.6 安装MQTT C语言开发库

#### 1. 国内源下载wiringPi库

```bash
apt-get update
# GitCode 镜像下载 wiring-pi-mirrors 仓库
git clone https://gitcode.com/gh_mirrors/wi/WiringPi.git
linux@ubuntu64-vm:~$ git clone https://gitcode.com/gh_mirrors/wi/WiringPi.git https://gitcode.com/sharpqian/wiring-pi-mirrors.git
Cloning into 'wiring-pi-mirrors'...
remote: Enumerating objects: 1733, done.
remote: Total 1733 (delta 0), reused 0 (delta 0), pack-reused 1733 (from 1)
Receiving objects: 100% (1733/1733), 806.66 KiB, done.
Resolving deltas: 100% (1158/1158), done.
```

#### 2. 交叉编译wiringPi库

（1）进入源码目录：

```bash
cd wiring-pi-mirrors
```

（2）修改Makefile以支持交叉编译。wiringPi的原始Makefile默认使用宿主的gcc，需要强制指定交叉编译器。执行以下命令修改Makefile：

```bash
sed -i 's/CC=gcc/CC=arm-none-linux-gnueabi-gcc/' Makefile
sed -i 's/AR=ar/AR=arm-none-linux-gnueabi-ar/' Makefile
sed -i 's/RANLIB=ranlib/RANLIB=arm-none-linux-gnueabi-ranlib/' Makefile
```

（3）编译：

```bash
make clean
make
```

#### 3. 准备Paho MQTT C库v1.0.2版本源码

通过 `git clone` 命令将整个仓库克隆到本地，之后通过 `git checkout` 切换到v1.0.2版本。

```bash
git clone https://gitee.com/mirrors/paho.mqtt.c.git
cd paho.mqtt.c
git checkout tags/v1.0.2 -b v1.0.2
```

#### 4. 交叉编译Paho MQTT C库

（1）安装cmake：

```bash
sudo apt-get install cmake
```

（2）交叉编译配置（使用Makefile）：

```bash
cd paho.mqtt.c
make clean
make CC=arm-none-linux-gnueabi-gcc AR=arm-none-linux-gnueabi-ar RANLIB=arm-none-linux-gnueabi-ranlib lib
```

### 3.1.7 创建工作文件夹

#### 1. 创建新文件夹

```bash
mkdir -p ~/mqt_files    # 工作文件夹
mkdir -p ~/nfs_share    # 共享文件夹
chmod 777 ~/nfs_share
```

#### 2. 复制文件到该文件夹

```bash
cp ~/wiring-pi-mirrors/wiringPi/libwiringPi.so.2.70 ~/mqt_files/
cp ~/paho.mqtt.c/build/output/libpaho-mqtt3a.so.1.0 ~/mqt_files/
cp ~/paho.mqtt.c/build/output/libpaho-mqtt3c.so.1.0 ~/mqt_files/
```

#### 3. 配置共享规则

```bash
mkdir -p ~/nfs_share
chmod 777 ~/nfs_share
sudo vi /etc/exports
```

在文件夹末尾添加一行。`*`表示允许任何IP访问；`rw`是读写权限；`no_root_squash`是让开发板的root用户在共享目录也有root权限，对调试比较友好；`sync`是同步写入；`no_subtree_check`可以提高性能。

```text
/home/linux/nfs_share *(rw,sync,no_root_squash,no_subtree_check)
```

```bash
sudo service nfs-kernel-server restart
```

## 3.2 程序编写及编译

### 3.2.1 mqt_iot编写

#### 1. 创建mqt_iot.c文件

编写程序代码并保存。

#### 2. 执行编译指令创建可执行文件

```bash
arm-none-linux-gnueabi-gcc -o mqt_iot mqt_iot.c \
  -I/home/linux/wiring-pi-mirrors/wiringPi \
  -I/home/linux/paho.mqtt.c/src \
  -L/home/linux/wiring-pi-mirrors/wiringPi \
  -L/home/linux/paho.mqtt.c/build/output \
  -lwiringPi -lpaho-mqtt3c -lpthread \
  -Wl,-rpath,.
```

### 3.2.2 DHT11驱动编写

1. 创建 `dht11_drv.c` 文件，编写程序代码并保存。

2. 创建 `Makefile`：

```make
obj-m := pwm_timer_drv.o dht11_drv.o
KERNEL_DIR := /home/linux/2023010804/linux-3.4.39
ARCH := arm
CROSS_COMPILE := arm-none-linux-gnueabi-

all:
	make -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
```

3. 编译：

```bash
make clean
make
```

## 3.3 连接实验箱与主机

### 3.3.1 配置Ubuntu虚拟机网络

```bash
sudo vi /etc/network/interfaces
```

```text
auto eth0
iface eth0 inet static
address 192.168.100.201
netmask 255.255.255.0
gateway 192.168.100.1
network 192.168.100.0
broadcast 192.168.100.255
dns-nameservers 192.168.100.1
```

### 3.3.2 配置实验箱网络

1. 设置MAC地址：

```bash
ifconfig eth0 hw ether 00:11:22:33:44:55
```

2. 配置IP地址（使用网关为 `192.168.100.1`）：

```bash
ifconfig eth0 192.168.100.128 netmask 255.255.255.0 up
```

虚拟机有线IP地址：`192.168.100.201`  
主机有线IPv4地址：`192.168.100.192`

3. 添加默认网关：

```bash
route add default gw 192.168.100.1
```

4. 验证配置：

```bash
ifconfig eth0
route -n
```

### 3.3.3 创建共享文件夹

1. 实验箱执行：

```bash
mkdir -p /mnt/nfs
mount -t nfs -o nolock 192.168.100.201:/home/linux/nfs_share /mnt/nfs
cd /mnt/nfs
export LD_LIBRARY_PATH=/mnt/nfs:$LD_LIBRARY_PATH
```

2. Ubuntu虚拟机执行：

```bash
cp dht11_drv.ko ~/nfs_share/
cp mqtt_iot ~/nfs_share/
```

## 3.4 运行程序测试结果

1. 下载dht11驱动程序：

```bash
insmod dht11_drv.ko
```

2. 运行mqtt程序：

```bash
./mqtt_iot
```

3. Ubuntu执行指令测试程序：

```bash
# 终端1：订阅数据
mosquitto_sub -h 192.168.100.201 -t "s5p6818/env/data"

# 终端2：发送控制指令
mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "led_on"
mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "buzzer_on"
mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "lcd_red"
```

# 四、实验结果及分析

## 4.1 本地运行测试

终端1（订阅）：

```bash
mosquitto_sub -h 10.153.143.200 -p 1883 -t test
```

终端2（发布）：

```bash
mosquitto_pub -h 10.153.143.200 -p 1883 -t test -m "mqtt server"
```

> 图1：本地运行测试

## 4.2 连接实验箱运行测试

### 4.2.1 共享文件夹结构

> 图2：共享文件夹结构

终端中可见 `./mqtt_iot` 输出：

```text
[INFO] Device init done. Motor IN1=2, IN2=3. All LEDs via /sys/class/leds/
[OK] DHT11: 0degree 0%
[INFO] MQTT connected
[INFO] subscribed: s5p6818/device/ctrl
[DATA] {"hum":0,"temperature":0}
[DATA] {"hum":0,"temperature":0}
...
```

`ls` 显示文件包括：

```text
break
dht11.ko
dht11_drv.ko
dht11_scan
dht11_scan.c
done
libpaho-mqtt3a.so
libpaho-mqtt3a.so.1.0
libpaho-mqtt3c.so
libpaho-mqtt3c.so.1.0
libwiringPi.so
libwiringPi.so.2.70
mqtt_iot
pwm_timer_drv.ko
scanner.log
sleep
```

### 4.2.2 LED灯控制

```bash
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "led_off"   # led关
```

> 图3：led灯关

```bash
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "led_on"    # led开
```

> 图4：led灯开

```bash
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "led1_off"  # led1关
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "led3_off"  # led3关
```

> 图5：led灯1、3关

```bash
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "led_on"    # led开
```

> 图6：led灯开

### 4.2.3 模拟数据回传

```bash
linux@ubuntu64-vm:~/mqtt_files$ mosquitto_sub -h 192.168.100.201 -t "s5p6818/env/data"  # 订阅数据
```

> 图7：回传传感器模拟数据

终端中可见DHT11读取错误信息，以及类似数据：

```text
[DATA] {"hum":60,"temperature":25}
[DATA] {"hum":62,"temperature":27}
[DATA] {"hum":66,"temperature":29}
```

### 4.2.4 传感器读数回传

```bash
linux@ubuntu64-vm:~/mqtt_files# mosquitto_sub -h 192.168.100.201 -t "s5p6818/env/data"  # 订阅数据
```

> 图8：回传传感器实际数据

终端中可见：

```text
[DATA] {"hum":0,"temperature":0}
[DATA] {"hum":0,"temperature":0}
[DATA] {"hum":0,"temperature":0}
...
```

### 4.2.5 蜂鸣器控制

```bash
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "buzzer_on"   # 蜂鸣器开
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "buzzer_off"  # 蜂鸣器关
```

### 4.2.6 Framebuffer液晶屏幕控制

```bash
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "lcd_red"    # 显示红色
```

> 图9：液晶屏显示红色

```bash
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "lcd_green"  # 显示绿色
```

> 图10：液晶屏显示绿色

```bash
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "lcd_blue"   # 显示蓝色
```

> 图11：液晶屏显示蓝色

```bash
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "lcd_white"  # 显示白色
```

> 图12：液晶屏显示白色

```bash
linux@ubuntu64-vm: $ mosquitto_pub -h 192.168.100.201 -t "s5p6818/device/ctrl" -m "lcd_black"  # 显示黑色
```

> 图13：液晶屏显示黑色