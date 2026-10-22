/* dht11.h — DHT11 温湿度采集 */
#ifndef DHT11_H
#define DHT11_H

/* 打开 /dev/dht11,缓存 fd。
 * 必须先 insmod /lib/modules/3.4.39-farsight/extra/dht11.ko。
 * 返回 0 成功,-1 失败。
 */
int dht11_init(void);
void dht11_release(void);

/* 阻塞读一次温湿度。
 *   *temp_int, *temp_dec: 温度整数/小数部分
 *   *humi_int, *humi_dec: 湿度整数/小数部分
 * 返回 0 成功,-1 失败。
 */
int dht11_read(int *temp_int, int *temp_dec,
               int *humi_int, int *humi_dec);

#endif /* DHT11_H */
