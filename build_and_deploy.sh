#!/bin/bash
# build_and_deploy.sh — 在 Ubuntu VM 上跑,编译 mqtt_iot 并部署到 /source/rootfs/root/mqtt_iot/
#
# 用法(在 VM 上):
#   chmod +x build_and_deploy.sh
#   ./build_and_deploy.sh
#
# 作用:
#   1. 用 arm-linux-gcc (4.5.1) 编译 src/ 下的源码
#   2. RPATH 设为 $ORIGIN,这样部署后 lab box 会在 binary 同目录找 lib
#   3. 部署到 /source/rootfs/root/mqtt_iot/(实验箱 NFS 根目录)
#   4. 带上 libmosquitto.so.0 和软链
#
# 依赖:
#   - /usr/local/toolchain/toolchain-4.5.1/bin/arm-linux-gcc
#   - install/{include,lib}/ (预编译的 ARM libmosquitto,见 build_libmosq.sh)
#   - /source/rootfs/ 写权限(必要时 sudo)

set -e
PROJ="$(cd "$(dirname "$0")" && pwd)"
SRC="$PROJ/src"
INSTALL="$PROJ/install"
DEPLOY=/source/rootfs/root/mqtt_iot

# 工具链
export PATH=/usr/local/toolchain/toolchain-4.5.1/bin:/usr/bin:/bin

cd "$SRC"

# clean
rm -f mqtt_iot *.o

# -lmosquitto 链接时找 libmosquitto.so,而仓库里只提交了版本化的 libmosquitto.so.0,
# 软链按需现建(Windows 上 checkout 成普通文件,所以不入库)
ln -sf libmosquitto.so.0 "$INSTALL/lib/libmosquitto.so"

# compile (单引号保住 $ORIGIN,不让 bash 展开)
echo "=== compile ==="
arm-linux-gcc -Wall -Wextra -O2 -std=gnu99 \
  -I. -I"$INSTALL/include" \
  main.c mqtt_client.c dht11.c led.c lcd.c servo.c beep.c \
  -L"$INSTALL/lib" -lmosquitto -lpthread \
  '-Wl,-rpath,$ORIGIN' \
  -o mqtt_iot

# verify
echo "=== rpath check ==="
arm-linux-readelf -d mqtt_iot | grep -E 'NEEDED|RPATH|RUNPATH'

# deploy (cp;权限不够时 sudo,密码从 stdin)
echo "=== deploy ==="
deploy_one() {
  if cp "$1" "$2" 2>/dev/null; then
    echo "  $1 -> $2"
  else
    echo "1" | sudo -S cp "$1" "$2" >/dev/null 2>&1
    echo "  $1 -> $2 (sudo)"
  fi
}

mkdir -p "$DEPLOY"
deploy_one "$SRC/mqtt_iot" "$DEPLOY/mqtt_iot"
deploy_one "$INSTALL/lib/libmosquitto.so.0" "$DEPLOY/libmosquitto.so.0"

( cd "$DEPLOY" && ln -sf libmosquitto.so.0 libmosquitto.so ) 2>/dev/null \
  || echo "1" | sudo -S bash -c "cd $DEPLOY && ln -sf libmosquitto.so.0 libmosquitto.so" >/dev/null 2>&1

echo "=== deployed ==="
ls -la "$DEPLOY/"
arm-linux-readelf -d "$DEPLOY/mqtt_iot" | grep -E 'NEEDED|RPATH|RUNPATH'
