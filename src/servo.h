/* servo.h — 舵机控制(ioctl 接口)
 *
 * 6818M4 出厂 servo_driver.c 提供的接口:
 *   open("/dev/servo", O_RDWR)
 *   ioctl(fd, cmd, 0)  cmd 取值见下
 *
 * 角度命令:
 *   ROTATE_0/45/90/135/180 — 预设角度
 *   SET_ANGLE               — 任意角度(0~180),arg 为角度值
 *
 * 失败时只 LOG_W,不阻塞主程序(允许 servo 模块未加载/设备节点缺失)。
 */
#ifndef SERVO_H
#define SERVO_H

/* ioctl 魔数与命令(从 6818M4/servo_driver.c 直接抄) */
#define SERVO_IOCTL_MAGIC 'S'
#define SERVO_ROTATE_0    _IO(SERVO_IOCTL_MAGIC, 0)
#define SERVO_ROTATE_45   _IO(SERVO_IOCTL_MAGIC, 1)
#define SERVO_ROTATE_90   _IO(SERVO_IOCTL_MAGIC, 2)
#define SERVO_ROTATE_135  _IO(SERVO_IOCTL_MAGIC, 3)
#define SERVO_ROTATE_180  _IO(SERVO_IOCTL_MAGIC, 4)
#define SERVO_SET_ANGLE   _IO(SERVO_IOCTL_MAGIC, 5)

/* 初始化:尝试打开 /dev/servo,失败记日志但不返回错误 */
int  servo_init(void);

/* 释放 fd */
void servo_release(void);

/* 转到指定角度(0..180,超出范围被驱动截断) */
int  servo_set_angle(int angle);

/* 转到预设档位(0/45/90/135/180) */
int  servo_goto(int slot);  /* slot: 0/1/2/3/4 → 0/45/90/135/180 */

#endif /* SERVO_H */
