set print pretty on
set print elements 4096 
set print vtbl on
cd ../../bin
pwd
file bqtd-ctp-d
set args --conf=config/bqtd-ctp/bqtd-ctp.yaml
handle SIGUSR1 noprint nostop
