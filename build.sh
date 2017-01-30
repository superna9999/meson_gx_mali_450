#!/bin/sh

# Takem from https://github.com/mripard/sunxi-mali/blob/master/build.sh
# Thanks Maxime !

DRIVER_DIR=$(pwd)/driver/src/devicedrv/mali/
USING_UMP=0 BUILD=debug MALI_DMA_BUF_MAP_ON_ATTACH=1 USING_PROFILING=0 MALI_PLATFORM=meson USING_DVFS=0 make -C $DRIVER_DIR
cp $DRIVER_DIR/mali.ko .
