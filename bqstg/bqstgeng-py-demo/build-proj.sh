#!/bin/bash
set -u
set -e

readonly SOLUTION_ROOT_DIR=/mnt/storage/work/betterquant2

black stgeng-10000.py
cp stgeng-10000.py $SOLUTION_ROOT_DIR/bin/
