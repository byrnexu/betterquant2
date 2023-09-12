set -xue

bash rsync-inc-of-stgeng.sh

cd lib
ls | grep 'stg\|algo' | xargs -t -i rm -rf {}
cd -

cd bin
ls | grep stg | xargs -t -i rm -rf {}
cd -

cd bqalgo && bash build-proj.sh && cd -
cd bqstg/bqstgengimpl && bash build-proj.sh && cd -
cd bqstg/bqstgeng-cxx && bash build-proj.sh && cd -
cd bqstg/bqstgeng-cxx && bash build-proj.sh && cd -

cd bqstg/bqstgeng-cxx && bash build-proj.sh && cd -
cd bqstg/bqstgeng-cxx-demo && bash build-proj.sh && cd -
cd bqstg/bqstgeng-cxx-demo-cn && bash build-proj.sh && cd -

cd bqstg/bqstgeng-py  && bash build-proj.sh && cd -
cd bqstg/bqstgeng-py-demo  && bash build-proj.sh && cd -
cd bqstg/bqstgeng-py-demo-cn  && bash build-proj.sh && cd -

cd bqstg/bqstg-manual && bash build-proj.sh && cd -
