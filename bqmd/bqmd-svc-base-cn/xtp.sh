set -xue

bash build-proj.sh

cd ../bqmd-xtp/
rm -rf ../../bin/bqmd-xtp-d*
bash build-proj.sh
cd -

cd ../../bin
./bqmd-xtp-d --conf=config/bqmd-xtp/bqmd-xtp.yaml
cd -
