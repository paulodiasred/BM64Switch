#!/bin/bash
source "$(dirname "$0")/common.sh"
"$BIN" > game.log 2>&1 &
PID=$!
sleep 6
WID=$(xdotool search --name "Bomberman" | head -1)
echo "window=$WID"
[ -n "$WID" ] && xdotool windowactivate --sync $WID windowmove $WID 0 0 2>/dev/null
# Script: list of "time:action" entries, action = key name or "shot"
t=6
for step in $STEPS; do
  at=${step%%:*}; act=${step#*:}
  sleep $((at - t)); t=$at
  kill -0 $PID 2>/dev/null || { echo "exited at ${t}s"; break; }
  echo "t=$t now_ms=$(($(date +%s%N)/1000000))" >> $RUN_DIR/shots.log
  if [ "$act" = "shot" ]; then import -window $WID ${PFX:-play}_${t}.png 2>/dev/null
  else xdotool keydown --window $WID $act; sleep 0.2; xdotool keyup --window $WID $act; fi
done
grep -E "VmHWM|VmRSS" /proc/$PID/status 2>/dev/null
kill $PID 2>/dev/null; sleep 1; kill -9 $PID 2>/dev/null
grep -v "Loading FileID" game.log | tail -15
