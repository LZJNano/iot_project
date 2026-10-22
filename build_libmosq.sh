#!/bin/bash
# build_libmosq.sh — 交叉编译 mosquitto 0.15 的客户端库 (ARM),产物放到 install/
#
# 用法(在 Ubuntu VM 上):
#   ./build_libmosq.sh
#
# 前置条件: mosquitto 0.15 的源码解压在 $MOSQ_DIR(改了下面的变量即可)
#
# 注意: 仓库里已经提交了编好的 install/lib/libmosquitto.so.0,
#       只有想自己重编时才需要跑这个脚本。

set -e
export PATH=/usr/local/toolchain/toolchain-4.5.1/bin:$PATH
MOSQ_DIR=/tmp/mosquitto-0.15
OUT_DIR="$(cd "$(dirname "$0")" && pwd)/install"
mkdir -p $OUT_DIR/lib $OUT_DIR/include

# 1. 编译 libmosquitto.so
cd $MOSQ_DIR/lib
# 覆盖默认 CC 和 CFLAGS,跳过 -lwrap (本项目不用 tcp wrappers)
make clean >/dev/null 2>&1 || true
make CC=arm-linux-gcc \
     CFLAGS="-I. -I.. -Wall -O2 -fPIC" \
     LIB_CFLAGS="-fPIC" \
     LIBS="" 2>&1 | tail -20
echo "===产物==="
ls -la libmosquitto.so*

# 2. 复制头文件
cp $MOSQ_DIR/lib/mosquitto.h $OUT_DIR/include/

# 3. 复制 .so
cp libmosquitto.so.0 $OUT_DIR/lib/
# 给 soname 建 symlink
ln -sf libmosquitto.so.0 $OUT_DIR/lib/libmosquitto.so

# 4. 同时编一个静态库版本 (--no-as-needed / 不依赖 so)
arm-linux-gcc -shared -Wl,-soname,libmosquitto.so.0 -o $OUT_DIR/lib/libmosquitto.so.0 \
    mosquitto.o logging_mosq.o memory_mosq.o messages_mosq.o net_mosq.o \
    read_handle.o read_handle_client.o read_handle_shared.o \
    send_mosq.o send_client_mosq.o util_mosq.o will_mosq.o 2>&1 || true

echo "===install==="
ls -la $OUT_DIR/lib/ $OUT_DIR/include/
