#!/bin/bash

packagedate=$(date +%Y%m%d%H%M%S)
echo $packagedate

lcurdir=$(readlink -f .)
SOURCE_PATH=$lcurdir
DEST_PATH0=$lcurdir/Mtk-image
project=$(cat out/buildflag.txt)
echo project=$project
option=$(cat out/options.txt)
DEST_PATH1=$DEST_PATH0/${project}_${option}_${packagedate}
DEST_PATH2=$DEST_PATH1/${project}_${option}_${packagedate}_image

mkdir $DEST_PATH0
mkdir $DEST_PATH1
mkdir $DEST_PATH2

cp out/target/product/$project/*_Android_scatter.txt $DEST_PATH2
cp out/target/product/$project/preloader_$project.bin $DEST_PATH2
cp out/target/product/$project/boot.img $DEST_PATH2
cp out/target/product/$project/cache.img $DEST_PATH2
cp out/target/product/$project/lk.bin $DEST_PATH2
cp out/target/product/$project/logo.bin $DEST_PATH2
cp out/target/product/$project/recovery.img $DEST_PATH2
cp out/target/product/$project/secro.img $DEST_PATH2
cp out/target/product/$project/system.img $DEST_PATH2
cp out/target/product/$project/userdata.img $DEST_PATH2
cp out/target/product/$project/trustzone.bin $DEST_PATH2

mv out/target/product/$project/full_$project-ota*.zip $DEST_PATH1/quanling_$packagedate.zip
mv out/target/product/$project/obj/PACKAGING/target_files_intermediates/full_$project-target_files*.zip $DEST_PATH1/ota_$packagedate.zip

mkdir $DEST_PATH2/database/
cp out/target/product/$project/system/etc/mddb/BPLGUInfoCustomAppSrcP_* $DEST_PATH2/database/
cp $(find out/target/product/$project/obj/CGEN/ -name "APDB_*" | grep -v ENUM) $DEST_PATH2/database/

mkdir  $DEST_PATH2/$project/
cp out/target/product/$project/system/data/misc/ProjectConfig.mk $DEST_PATH2/$project/
cp out/target/product/$project/system/build.prop $DEST_PATH2/$project/
cp vendor/mediatek/proprietary/bootable/bootloader/preloader/custom/$project/inc/custom_MemoryDevice.h $DEST_PATH2/$project/

