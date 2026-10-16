handle SIGUSR1 SIGUSR2 SIGPIPE nostop noprint
run
bt 5
x/14i $pc-40
info registers
