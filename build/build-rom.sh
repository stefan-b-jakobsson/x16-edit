#!/bin/bash
BUILD_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
SRC_PATH="$BUILD_PATH"/..

cl65 -v --asm-args -Dtarget_mem=2 -o "$BUILD_PATH/x16edit-rom.bin" -t cx16 -C x16edit-rom.cfg --mapfile x16edit-rom.map -vm  $SRC_PATH/main.asm