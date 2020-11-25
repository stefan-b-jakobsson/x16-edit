#!/bin/bash
BUILD_PATH="./"
SRC_PATH="$BUILD_PATH"/..

cl65 -v --asm-args -Dtarget_mem=1 -o "$BUILD_PATH/x16edit.prg" -u __EXEHDR__ -t cx16 -C cx16-asm.cfg $SRC_PATH/main.asm