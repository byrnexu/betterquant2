#!/bin/bash
set -u
set -e

readonly SOLUTION_ROOT_DIR=/mnt/storage/work/bitquant2

black stgeng-1000*.py
cp stgeng-1000*.py $SOLUTION_ROOT_DIR/bin/
