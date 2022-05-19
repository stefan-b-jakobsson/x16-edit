#!/bin/bash
BUILD_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
SRC_PATH="$BUILD_PATH"/..

cl65 -v --asm-args -Dtarget_mem=1 -o "$BUILD_PATH/X16EDIT-HI.PRG" -t cx16 -C x16edit-hiram.cfg $SRC_PATH/main.asm