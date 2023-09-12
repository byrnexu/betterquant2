set -xue

rm -rf ../../bin/plugin/libbqtd-srv-risk-plugin-pnl-monitor*

rm -rf ../../bin/bqtd-srv-d
rm -rf ../../bin/bqtd-srv

bash build-proj.sh
cd ../bqtd-srv/

bash build-proj.sh
cd -
