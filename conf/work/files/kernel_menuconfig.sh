#! /bin/bash

PWD=`pwd`
export PATH=$PATH:${PWD}/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/
PROD_OUT="$PWD/out/target/product/sp9820w_6c10"
KERNEL_OUT="$PROD_OUT/obj/KERNEL"
DIR_ROOT="$PWD/vendor/amoi/root"
DIR_ROOT_KERNEL_MODULES="$DIR_ROOT/lib/modules"
SH_CREATE_USER_CONFIG="${PWD}/kernel/scripts/sprd_create_user_config.sh"
TARGET_DEVICE_LOW_RAM_CONFIG="${PWD}/vendor/amoi/config/low_ram_diff_config"
DIR_KERNEL_MODULE_SPRDWL="$PWD/vendor/sprd/wcn/wifi/sc2331"
DIR_KERNEL_MODULE_MALI="$PWD/vendor/sprd/open-source/libs/mali/driver/mali"
DIR_AMOI_TOOL="${PWD}/vendor/amoi/tools"
TOOL_DTB="$DIR_AMOI_TOOL/dtbTool"
TOOL_MKBOOTFS="$DIR_AMOI_TOOL/mkbootfs"
TOOL_MINIGZIP="$DIR_AMOI_TOOL/minigzip"
TOOL_MKBOOTIMG="$DIR_AMOI_TOOL/mkbootimg"
CMDLINE="console=ttyS1,115200n8"


DT_START=`date +%x%A%X`
echo "=================== build start at ${DT_START}============"
mkdir -p ${KERNEL_OUT}
make ARCH=arm -C kernel O=$KERNEL_OUT menuconfig
