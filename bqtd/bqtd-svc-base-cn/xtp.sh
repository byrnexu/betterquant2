set -xue

bash build-proj.sh

cd ../bqtd-xtp/
rm -rf ../../bin/bqtd-xtp-d*
bash build-proj.sh
cd -

cd ../../bin
./bqtd-xtp-d --conf=config/bqtd-xtp/bqtd-xtp.yaml
cd -
