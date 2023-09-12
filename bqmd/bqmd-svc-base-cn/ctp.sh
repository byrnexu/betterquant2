set -xue

bash build-proj.sh

cd ../bqmd-ctp/
rm -rf ../../bin/bqmd-ctp-d*
bash build-proj.sh
cd -

cd ../../bin
./bqmd-ctp-d --conf=config/bqmd-ctp/bqmd-ctp.yaml
cd -
