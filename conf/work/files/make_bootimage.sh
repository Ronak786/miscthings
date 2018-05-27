#! /bin/bash

set -e

PWD=`pwd`
export PATH=$PATH:${PWD}/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/
export PROD_OUT="$PWD/out/target/product/sp9820w_6c10"
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
make ARCH=arm -C kernel O=$KERNEL_OUT sp9820w_6c10_defconfig
$SH_CREATE_USER_CONFIG ${KERNEL_OUT}/.config ${PWD}/vendor/amoi/config/low_ram_diff_config
make -C kernel O=$KERNEL_OUT ARCH=arm CROSS_COMPILE=arm-eabi- headers_install
make -C kernel O=$KERNEL_OUT ARCH=arm CROSS_COMPILE=arm-eabi- -j4

echo "=================== build kernel modules================"
make -C kernel O=$KERNEL_OUT ARCH=arm CROSS_COMPILE=arm-eabi- modules
make -C $DIR_KERNEL_MODULE_SPRDWL ARCH=arm CROSS_COMPILE=arm-eabi- SPRDWL_PLATFORM=sc8830 USING_PP_CORE=2 DFS_MAX_FREQ= DFS_MIN_FREQ=  KDIR=$KERNEL_OUT CUSTOM_EXTRA=
make ARCH=arm -C $KERNEL_OUT M=$DIR_KERNEL_MODULE_SPRDWL
make -C  $DIR_KERNEL_MODULE_MALI ARCH=arm CROSS_COMPILE=arm-eabi- MALI_PLATFORM=sc8830 USING_PP_CORE=2  KDIR=$KERNEL_OUT
make ARCH=arm -C $KERNEL_OUT M=$DIR_KERNEL_MODULE_MALI modules

echo "=================== copy kernel modules to root directory====="
find ${KERNEL_OUT} -name *.ko ! -name mali.ko | xargs -I{} cp {} ${DIR_ROOT_KERNEL_MODULES}
cp ${DIR_KERNEL_MODULE_SPRDWL}/sprdwl.ko ${DIR_ROOT_KERNEL_MODULES}
cp ${DIR_KERNEL_MODULE_MALI}/mali.ko ${DIR_ROOT_KERNEL_MODULES}
find ${DIR_ROOT_KERNEL_MODULES} -name *.ko ! -name mali.ko ! -name sprdwl.ko -exec arm-eabi-strip -d --strip-unneeded {} \;

echo "=================== generate dt.img,ramdisk.img,boot.img====="
$TOOL_DTB -o ${PROD_OUT}/dt.img -s 2048 -p ${KERNEL_OUT}/scripts/dtc/ ${KERNEL_OUT}/arch/arm/boot/dts/
#$TOOL_MKBOOTFS $DIR_ROOT | $TOOL_MINIGZIP > ${PROD_OUT}/ramdisk.img
(cd vendor/amoi/root; find . | cpio -o -H newc | gzip > ${PROD_OUT}/ramdisk.img )
$TOOL_MKBOOTIMG  --kernel ${KERNEL_OUT}/arch/arm/boot/Image --ramdisk ${PROD_OUT}/ramdisk.img --cmdline "${CMDLINE}" --base 0x00000000 --pagesize 2048 --dt ${PROD_OUT}/dt.img  --output ${PROD_OUT}/boot.img

DT_END=`date +%x%A%X`
echo "=================== build end at ${DT_END}================"
exit 0
