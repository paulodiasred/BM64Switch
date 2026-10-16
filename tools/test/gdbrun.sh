#!/bin/bash
source "$(dirname "$0")/common.sh"
CMDS=${GDBCMDS:-$SRC/tools/test/gdb_bt.cmd}
timeout ${TMO:-60} gdb -batch -x $CMDS "$BIN" 2>&1 | grep -v "^\[New Thread\|^\[Thread .* exited" | tail -${LINES_OUT:-80}
