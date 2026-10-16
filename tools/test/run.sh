#!/bin/bash
source "$(dirname "$0")/common.sh"
"$BIN" > game.log 2>&1 &
PID=$!
for t in ${SHOTS:-10 20 30}; do
  sleep $((t - ${prev:-0})); prev=$t
  import -window root shot_${t}.png 2>/dev/null
  kill -0 $PID 2>/dev/null || { echo "exited early at ${t}s"; break; }
done
kill $PID 2>/dev/null; sleep 1; kill -9 $PID 2>/dev/null
tail -40 game.log
