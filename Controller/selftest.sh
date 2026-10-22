#!/bin/bash
# selftest.sh - end-to-end test of Controller against local broker
# flow: mosquitto_sub (listen ctrl), Controller (xvfb),
#       mosquitto_pub simulate device publishing env/data,
#       mosquitto_pub simulate button-press publishing device/ctrl,
#       finally inspect both logs.
set -e
cd "$HOME/Controller"

# Clear old logs
: > /tmp/ctrl.log
: > /tmp/ctrlr.log

echo "[TEST] === starting mosquitto_sub on s5p6818/device/ctrl ==="
nohup mosquitto_sub -h 127.0.0.1 -p 1883 -t s5p6818/device/ctrl -v \
  > /tmp/ctrl.log 2>&1 &
SUB_PID=$!
echo "[TEST] sub pid=$SUB_PID"

sleep 1

echo "[TEST] === launching Controller under xvfb-run (broker=127.0.0.1, base=s5p6818) ==="
nohup xvfb-run -a ./Controller > /tmp/ctrlr.log 2>&1 &
CTRL_PID=$!
echo "[TEST] controller pid=$CTRL_PID"

sleep 5

echo "[TEST] === mosquitto_pub simulating device publishing env/data ==="
mosquitto_pub -h 127.0.0.1 -p 1883 -t s5p6818/env/data -m '{"hum":55,"temperature":24}'
mosquitto_pub -h 127.0.0.1 -p 1883 -t s5p6818/env/data -m '{"hum":60,"temperature":25}'
sleep 2

echo "[TEST] === mosquitto_pub simulating Controller button-press (pub device/ctrl) ==="
# Controller is GUI; cannot click in xvfb, so we directly publish
# what the buttons would publish, verifying broker <-> topic wiring.
mosquitto_pub -h 127.0.0.1 -p 1883 -t s5p6818/device/ctrl -m led1_on
mosquitto_pub -h 127.0.0.1 -p 1883 -t s5p6818/device/ctrl -m led3_off
mosquitto_pub -h 127.0.0.1 -p 1883 -t s5p6818/device/ctrl -m led_on
mosquitto_pub -h 127.0.0.1 -p 1883 -t s5p6818/device/ctrl -m servo_90
mosquitto_pub -h 127.0.0.1 -p 1883 -t s5p6818/device/ctrl -m lcd_red
sleep 1

echo
echo "================================================="
echo "[RESULT] /tmp/ctrl.log (mosquitto_sub on device/ctrl):"
echo "================================================="
cat /tmp/ctrl.log
echo
echo "================================================="
echo "[RESULT] /tmp/ctrlr.log (Controller stderr, non-font lines):"
echo "================================================="
grep -av 'Fontconfig warning' /tmp/ctrlr.log | grep -av '^$' || echo '(no app output yet)'

echo
echo "[TEST] cleanup"
kill $CTRL_PID 2>/dev/null || true
kill $SUB_PID 2>/dev/null || true
sleep 1
echo "[TEST] done"