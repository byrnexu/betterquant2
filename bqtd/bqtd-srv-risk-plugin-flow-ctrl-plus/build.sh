set -xue

rm -rf ../../lib/libbqriskctrl*

rm -rf ../../bin/plugin/libbqtd-srv-risk-plugin-flow-ctrl-plus*

rm -rf ../../bin/bqtd-srv-d
rm -rf ../../bin/bqtd-srv

cd ../../bqriskctrl
bash build-proj.sh
cd -

bash build-proj.sh
cd ../bqtd-srv/

bash build-proj.sh
cd -
