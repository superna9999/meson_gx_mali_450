#!/bin/sh

# Takem from https://github.com/mripard/sunxi-mali/blob/master/build.sh
# Thanks Maxime !

DRIVER_DIR=$(pwd)/driver/src/devicedrv/mali/
USING_UMP=0 BUILD=release USING_PROFILING=0 MALI_PLATFORM=meson USING_DVFS=0 make -C $DRIVER_DIR
cp $DRIVER_DIR/mali.ko .
