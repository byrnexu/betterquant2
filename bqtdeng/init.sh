docker run -d --name bqtdeng -h tdengine -p 6041:6041 -p 6030-6035:6030-6035 -p 6030-6035:6030-6035/udp \
  -v /mnt/storage/betterquant/taos/log:/var/log/taos \
  -v /mnt/storage/betterquant/taos/data:/var/lib/taos \
  tdengine/tdengine:latest   
