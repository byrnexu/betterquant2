#!/bin/bash
set -u
set -e

readonly PARALLEL_COMPILE_THREAD_NUM=6
readonly SOLUTION_ROOT_DIR=/mnt/storage/work/betterquant2

readonly PYTHON_INC_DIR=/usr/include/python3.8

readonly PROJ_NAME=$(pwd | awk -F'/' '{print $NF}' | awk -F'-' '{print $1}')
readonly FILE_OF_CPP="\.cpp$\|\.cc$\|\.hpp$\|\.h$"

check_command_format() {
  readonly CORRECT_COMMAND_FORMAT='bash build.sh or bash build.sh all'
  [[ $# != 0 && $# != 1     ]] && echo usage: $CORRECT_COMMAND_FORMAT && exit 1
  [[ $# == 1 && $1 != "all" ]] && echo usage: $CORRECT_COMMAND_FORMAT && exit 1
  echo $0 $*
}
check_command_format $*

format_src_code() {
  if [[ -d $1 ]]; then
    find $1 -type f -mmin -60 | grep $FILE_OF_CPP | xargs -t -i clang-format -i {}
  fi
}

build() {
  echo build $1 version
  build_type=$1
  build_type="${build_type^}"

  mkdir -p build/$1 || exit 1
  cd build/$1

  cmake ../../ -DCMAKE_BUILD_TYPE=$build_type \
    -DSOLUTION_ROOT_DIR:STRING=${SOLUTION_ROOT_DIR} \
    -DPYTHON_INC_DIR:STRING=${PYTHON_INC_DIR} \
    || (cd - && exit 1)

  if [[ $# -gt 1 ]]; then
    make -j $PARALLEL_COMPILE_THREAD_NUM $2 || (cd - && exit 1)
  else
    make -j $PARALLEL_COMPILE_THREAD_NUM    || (cd - && exit 1)
  fi

  cd -
}

main() {
  black src/stgeng.py
  black src/status_code.py

  cp src/stgeng.py $SOLUTION_ROOT_DIR/bin/
  cp src/status_code.py $SOLUTION_ROOT_DIR/bin/

  mkdir -p $SOLUTION_ROOT_DIR/bin/config/bqstgeng-py-demo/
  cp config/bqstgeng-py-demo.yaml \
    $SOLUTION_ROOT_DIR/bin/config/bqstgeng-py-demo/bqstgeng-py-demo.yaml

  mkdir -p $SOLUTION_ROOT_DIR/bin/config/bqstgeng-py-demo-cn/
  cp config/bqstgeng-py-demo.yaml \
    $SOLUTION_ROOT_DIR/bin/config/bqstgeng-py-demo-cn/bqstgeng-py-demo-cn.yaml

  cp src/stgeng.py $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo/
  cp src/status_code.py $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo/
  mkdir -p $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo/config/
  cp config/bqstgeng-py-demo.yaml \
    $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo/config/

  cp src/stgeng.py $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo-cn/
  cp src/status_code.py $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo-cn/
  mkdir -p $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo-cn/config/
  cp config/bqstgeng-py-demo.yaml \
    $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo-cn/config/bqstgeng-py-demo-cn.yaml

  format_src_code inc/
  format_src_code src/
  build debug $PROJ_NAME

  format_src_code bench/
  format_src_code test/

  cd build/debug
  make -j $PARALLEL_COMPILE_THREAD_NUM
  cp $SOLUTION_ROOT_DIR/lib/lib$PROJ_NAME-d.so \
     $SOLUTION_ROOT_DIR/bin/$PROJ_NAME.so
  cp $SOLUTION_ROOT_DIR/lib/lib$PROJ_NAME-d.so \
     $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo/$PROJ_NAME.so
  cp $SOLUTION_ROOT_DIR/lib/lib$PROJ_NAME-d.so \
     $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo-cn/$PROJ_NAME.so
  make tests
  cd -

  [[ $# != 1 || $1 != "all" ]] && exit
  build release
  cd build/release
  cp $SOLUTION_ROOT_DIR/lib/lib$PROJ_NAME.so \
     $SOLUTION_ROOT_DIR/bin/$PROJ_NAME.so
  cp $SOLUTION_ROOT_DIR/lib/lib$PROJ_NAME.so \
     $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo/$PROJ_NAME.so
  cp $SOLUTION_ROOT_DIR/lib/lib$PROJ_NAME.so \
     $SOLUTION_ROOT_DIR/bqstg/bqstgeng-py-demo-cn/$PROJ_NAME.so
  make bench
  cd -
}

main $*
