/* beep.h — 蜂鸣器控制(ioctl 接口)
 *
 * 6818M4/beep_driver.c 提供的接口:
 *   open("/dev/beep", O_RDWR)
 *   ioctl(fd, BEEP_ON, 0)
 *   ioctl(fd, BEEP_OFF, 0)
 *   ioctl(fd, SET_FREQUENCY, &hz)  hz 是 0..100(占空比/频率参数,见驱动)
 *
 * 失败时只 LOG_W,不阻塞主程序。
 */
#ifndef BEEP_H
#define BEEP_H

#define BEEP_IOCTL_MAGIC 'B'
#define BEEP_ON          _IO(BEEP_IOCTL_MAGIC, 0)
#define BEEP_OFF         _IO(BEEP_IOCTL_MAGIC, 1)
#define BEEP_SET_FREQ    _IOW(BEEP_IOCTL_MAGIC, 2, int)

int  beep_init(void);
void beep_release(void);
int  beep_on(void);
int  beep_off(void);
int  beep_set_freq(int hz);  /* 0..100,驱动内部截断 */

#endif /* BEEP_H */
