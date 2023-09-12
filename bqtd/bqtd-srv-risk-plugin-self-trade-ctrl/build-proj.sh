#!/bin/bash
set -u
set -e

readonly PARALLEL_COMPILE_THREAD_NUM=6
readonly SOLUTION_ROOT_DIR=/mnt/storage/work/bitquant2

readonly PLUGIN_NO=$(cat CMakeLists.txt | grep -i 'project(' | \
  awk '{print $1}' | awk -F'-' '{print $NF}')

readonly PROJ_NAME=$(pwd | awk -F'/' "{print \$NF\"-$PLUGIN_NO\"}")
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
    -DSOLUTION_ROOT_DIR:STRING=${SOLUTION_ROOT_DIR} || (cd - && exit 1)

  if [[ $# -gt 1 ]]; then
    make -j $PARALLEL_COMPILE_THREAD_NUM $2 || (cd - && exit 1)
  else
    make -j $PARALLEL_COMPILE_THREAD_NUM    || (cd - && exit 1)
  fi

  cd -
}

sync_plugin() {
  mkdir -p $SOLUTION_ROOT_DIR/bin/plugin/acctId/
  rsync -aPc $SOLUTION_ROOT_DIR/bin/plugin/global/*$PROJ_NAME* \
             $SOLUTION_ROOT_DIR/bin/plugin/acctId/
}

main() {
  format_src_code inc/
  format_src_code src/
  build debug $PROJ_NAME

  format_src_code bench/
  format_src_code test/

  cd build/debug
  make -j $PARALLEL_COMPILE_THREAD_NUM
  make tests
  cd -
  sync_plugin

  [[ $# != 1 || $1 != "all" ]] && exit
  build release
  cd build/release
  make bench
  cd -
  sync_plugin
}

main $*
