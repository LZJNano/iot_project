/* lcd.c — LCD framebuffer 实现
 *
 * 基于 /source/rootfs/fs6818b_test/LCD/lcd.c,精简掉 main()
 * 和复杂的画线/画矩形(本项目只用到 fill_screen)。
 *
 * 设备: /dev/fb0
 * 操作: open / ioctl(FBIOGET_FSCREENINFO/VSCREENINFO) / mmap
 */
#include "lcd.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

static int g_fbfd = -1;
static struct fb_var_screeninfo g_vinfo;
static struct fb_fix_screeninfo g_finfo;
static long g_screensize = 0;
static char *g_fbp = NULL;

int lcd_init(void)
{
    g_fbfd = open(LCD_FB_PATH, O_RDWR);
    if (g_fbfd < 0) {
        LOG_E("open %s failed: %s", LCD_FB_PATH, strerror(errno));
        return -1;
    }

    if (ioctl(g_fbfd, FBIOGET_FSCREENINFO, &g_finfo)) {
        LOG_E("ioctl FBIOGET_FSCREENINFO failed: %s", strerror(errno));
        close(g_fbfd);
        g_fbfd = -1;
        return -1;
    }
    if (ioctl(g_fbfd, FBIOGET_VSCREENINFO, &g_vinfo)) {
        LOG_E("ioctl FBIOGET_VSCREENINFO failed: %s", strerror(errno));
        close(g_fbfd);
        g_fbfd = -1;
        return -1;
    }

    LOG_I("LCD %dx%d, %dbpp", g_vinfo.xres, g_vinfo.yres, g_vinfo.bits_per_pixel);

    g_screensize = (long)g_vinfo.xres * g_vinfo.yres * g_vinfo.bits_per_pixel / 8;

    g_fbp = (char *)mmap(0, g_screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                         g_fbfd, 0);
    if (g_fbp == (char *)-1) {
        LOG_E("mmap framebuffer failed: %s", strerror(errno));
        g_fbp = NULL;
        close(g_fbfd);
        g_fbfd = -1;
        return -1;
    }
    return 0;
}

void lcd_release(void)
{
    if (g_fbp) {
        munmap(g_fbp, g_screensize);
        g_fbp = NULL;
    }
    if (g_fbfd >= 0) {
        close(g_fbfd);
        g_fbfd = -1;
    }
}

void lcd_fill_screen(color_t color)
{
    if (!g_fbp) {
        LOG_W("lcd_fill_screen called before init");
        return;
    }
    if (g_vinfo.bits_per_pixel != 32) {
        LOG_W("only 32bpp supported, got %d", g_vinfo.bits_per_pixel);
        return;
    }
    int x, y;
    long location;
    int xres = g_vinfo.xres;
    int yres = g_vinfo.yres;
    for (y = 0; y < yres; y++) {
        for (x = 0; x < xres; x++) {
            location = (x + g_vinfo.xoffset) * (g_vinfo.bits_per_pixel / 8)
                     + (y + g_vinfo.yoffset) * g_finfo.line_length;
            *((unsigned long *)(g_fbp + location)) = color;
        }
    }
}

int lcd_fill_named(const char *name)
{
    if (!name) return -1;
    color_t c;
    if      (!strcmp(name, "red"))   c = RED;
    else if (!strcmp(name, "green")) c = GREEN;
    else if (!strcmp(name, "blue"))  c = BLUE;
    else if (!strcmp(name, "white")) c = WHITE;
    else if (!strcmp(name, "black")) c = BLACK;
    else if (!strcmp(name, "yellow"))c = YELLOW;
    else {
        LOG_W("unknown lcd color name: %s", name);
        return -1;
    }
    LOG_I("LCD fill: %s (0x%06lx)", name, (unsigned long)c);
    lcd_fill_screen(c);
    return 0;
}
