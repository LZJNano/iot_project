/* logger.h — 简易日志宏
 *
 * 用法:
 *   LOG_I("connected to %s:%d", host, port);
 *   LOG_E("open %s failed: %s", path, strerror(errno));
 *
 * 输出格式: [2026-09-20 19:30:01] [INFO ] connected to 192.168.1.105:1883
 * 不引入第三方日志库,纯 fprintf(stderr,) + localtime。
 *
 * 日志统一打到 stderr,stdout 留给以后可能的重定向(虽然本项目不用)。
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define LOG_I(...) _log_write("INFO ", __VA_ARGS__)
#define LOG_W(...) _log_write("WARN ", __VA_ARGS__)
#define LOG_E(...) _log_write("ERROR", __VA_ARGS__)

static inline void _log_write(const char *level, const char *fmt, ...)
{
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm);

    fprintf(stderr, "[%s] [%s] ", ts, level);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
    fflush(stderr);
}

#endif /* LOGGER_H */
