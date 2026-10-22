# Makefile — mqtt_iot
#
# 用法:
#   make              编译
#   make clean        清理
#   make deploy       编译 + 复制到 /source/rootfs/root/mqtt_iot/
#
# 工具链: arm-linux-* (Ubuntu 上 /usr/local/toolchain/toolchain-4.5.1/)
#   在 Ubuntu 上执行前需:
#     export PATH=/usr/local/toolchain/toolchain-4.5.1/bin:$PATH
# 依赖:
#   libmosquitto (Ubuntu 包 libmosquitto0-dev)
#   Linux pthread / framebuffer / sysfs (内核头文件)

CC       = arm-linux-gcc
INSTALL_DIR = $(abspath ../install)
CFLAGS   = -Wall -Wextra -O2 -std=gnu99 -I. -I$(INSTALL_DIR)/include
# -Wl,-rpath,'$$ORIGIN' 让二进制在自己所在目录找 lib,这样部署后不用设 LD_LIBRARY_PATH
LDFLAGS  = -L$(INSTALL_DIR)/lib -lmosquitto -lpthread -Wl,-rpath,'$$ORIGIN'

TARGET   = mqtt_iot
SRCS     = main.c mqtt_client.c dht11.c led.c lcd.c servo.c beep.c
OBJS     = $(SRCS:.c=.o)

DEPLOY_DIR = /source/rootfs/root/mqtt_iot

.PHONY: all clean deploy

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

deploy: $(TARGET)
	sudo mkdir -p $(DEPLOY_DIR)
	sudo cp $(TARGET) $(DEPLOY_DIR)/
	@echo "Deployed to $(DEPLOY_DIR)/$(TARGET)"
