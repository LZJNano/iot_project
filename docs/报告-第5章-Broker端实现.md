# 5. Broker 端实现

在成功实现设备端软件后,我们接下来就可以搭建 Broker 服务器了。我们已经在设备端软件中封装了 Mosquitto 0.15,因此我们的服务端也会采用 Mosquitto 0.15。

在前文第三章 1.4 节 MQTT broker 配置章节中,我们已对 Mosquitto 的安装和参数进行了详细的介绍,因此这里不再重复赘述,仅对服务端内的数据结构和数据流进行阐述。

## 5.1 Broker 数据结构

### 5.1.1 客户连接表

Broker 服务器的客户连接表上有两个 client_id,分别是 `fs6818_mqtt_iot` 和 `Controller`。前者代表的是实验箱,其 TCP socket 为 TCP to 192.168.100.111,也就是实验箱在局域网内的 IP 地址;订阅列表为 `s5p6818/device/ctrl`,也就是控制端下发指令的 topic。

而后者则代表的是控制端软件。其 TCP socket 为 TCP to 192.168.100.x,订阅列表为 `s5p6818/#`,也就是实验箱发来的 DHT11 传感器的温湿度数据。

| client_id | TCP socket | 订阅列表 |
| --- | --- | --- |
| fs6818_mqtt_iot | TCP to 192.168.100.111 | s5p6818/device/ctrl |
| Controller | TCP to 192.168.100.x | s5p6818/# |

表 5.1.1 客户连接表

### 5.1.2 主题订阅反索引表

主题订阅反索引表以主题模式为键,值为订阅者列表。broker 收到 PUBLISH 时直接查表就能找出所有订阅者。反索引的维护在 SUBSCRIBE / UNSUBSCRIBE 报文到达时动态更新。

| 主题模式 | 订阅者(client_id)列表 |
| --- | --- |
| s5p6818/# | Controller |
| s5p6818/device/ctrl | fs6818_mqtt_iot |

表 5.1.2 主题映射反索引表

## 5.2 数据流

我们的 broker 服务器的数据流按方向分为两路:温湿度上行(设备 → broker → Controller)和控制指令下行(Controller → broker → 设备)。

### 5.2.1 上行数据

上行数据传输流程主要包括实验箱传输数据、broker 服务器转发数据、以及 Controller 控制端接收数据。实验箱上传输数据的流程我们在前文中已经详细介绍,Controller 控制端接收数据的流程我们将在后文进行介绍,因此这里着重阐述 broker 服务器上的数据流。

我们的 broker 客户端收到来自实验箱的 PUBLISH 报文后,会先解析报文头,提取主题字段(`s5p6818/env/data`)和 payload 字段(JSON 格式的温湿度数据)。然后,broker 会在自己的反索引表中查找这个主题的订阅者。如表 5.1.2 所示,`s5p6818/#` 模式的订阅者是 Controller,`s5p6818/env/data` 与该通配模式匹配,因此 broker 知道 Controller 应当收到这条数据。

找到订阅者后,broker 会对 Controller 的 TCP socket 发起一次 PUBLISH 报文转发。Controller 端的 QtMqtt 客户端在收到这条转发报文后,会通过信号槽机制通知上层 UI 组件,UI 更新温湿度 label 显示。整个上行链路的延迟在同网段下实测小于 50 ms。

需要说明的是,本项目上行数据采用 QoS 0(最多一次)等级,允许偶发丢失,因为温湿度数据每 5 秒就有一条,偶发丢失可由下一条数据补偿。如果改为 QoS 1,broker 与 Controller 之间还会多一轮 PUBACK 确认,延迟会增加到 100 ms 量级,但可靠性更高。

### 5.2.2 下行数据

下行数据传输流程与上行类似,但方向相反,主要包括控制端传输指令、broker 服务器转发指令、以及设备端接收并执行指令。控制端发送指令的流程将在后文介绍,设备端接收指令并执行的流程我们在前文也已经详细介绍,这里同样着重阐述 broker 服务器上的数据流。

当 Controller 程序把用户点击按钮对应的字符串(如 `lcd_red`)包装成 PUBLISH 报文发到 broker 后,broker 会先解析报文头,提取主题字段 `s5p6818/device/ctrl`。然后,broker 在反索引表中查找这个主题的订阅者,根据表 5.1.2,匹配到的订阅者是 `fs6818_mqtt_iot`,也就是实验箱。

找到订阅者后,broker 会对实验箱的 TCP socket 发起一次 PUBLISH 报文转发。实验箱 `mqtt_iot` 的 loop 线程在 `mosquitto_loop(100)` 中收到这条报文,触发 `on_message` 回调,回调里把 payload 复制为字符串后调 `on_ctrl_message()`(在 main.c 中)分发到具体硬件设备,最终 LCD 屏变红。

下行数据采用 QoS 1 等级,以保证控制指令至少一次到达设备端。broker 在转发 PUBLISH 后会等待设备端的 PUBACK 确认,若在 keepalive × 1.5 时间内未收到确认,会重新发送一次。这意味着设备端有可能收到重复的控制命令,不过本项目中的硬件操作(如 `led_set_all(1)`)天然幂等,重复执行不会产生副作用。

实测整个下行链路从用户点击按钮到 LCD 屏变红,延迟小于 100 ms,完全满足实时控制的需求。

## 5.3 本章小结

本章对 broker 服务器端的数据结构和数据流进行了详细阐述。broker 在本系统中起到"消息中转"作用,通过维护客户连接表和主题订阅反索引表,实现按主题匹配的消息路由。本项目利用 Mosquitto 0.15 作为 broker,其仅需极简配置即可运行,体现了 MQTT 协议的易用性。

通过 broker 这一中间层,设备端和控制端完全解耦,双方不需要知道对方的网络地址,只需要按主题与 broker 通信即可。这种解耦设计是本系统最具扩展性的地方:后续要加入新的客户端(如数据持久化服务、告警服务、第三方云平台对接),只需要订阅相应主题即可,无需改动设备端或控制端的代码。
