#!/bin/bash

OUT_DIR="/home/sora/gitbase/KivviOSApplication_release/"
BASE_DIR="/home/sora/gitbase/KivviOSApplication/PreApplication/"
MK6737TSRC_DIR="/home/sora/gitbase/pangu_mt6737/device/teksun/tek6737t_36_m0/cynovo/"
MK6582SRC_DIR="/home/sora/gitbase/pangu_mt6582/mediatek/config/pangu/cynovo/"

SUB_PROJECTS=$(ls $BASE_DIR)
export JAVA_HOME="/home/sora/app/android-studio/jre"

option="$1"
if [ "x$option"  == "x" ]
then
    echo "need a option (debug|release|clean)"
    exit
fi

function copyfile() {
    local filename=$1
    local origfile=$2
    local subdir=$3
    cp $origfile ${OUT_DIR}${filename}.apk

    local basedir=
    case $subdir in
        KivviAssistant)
            basedir=kivviassistant
            ;;
        KivviLauncher)
            basedir=kivvilauncher
            ;;
        KivviDeviceService)
            basedir=kivvideviceservice
            ;;
        KivviSettings)
            basedir=kivvisettings
            ;;
        KivviSystemService)
            basedir=kivvisystemservice
            ;;
        *)
            echo "can not find copyable destination for building subdir $subdir"
            exit 1
            ;;
    esac
    cp $origfile ${MK6737TSRC_DIR}/${basedir}/${filename}.apk
    cp $origfile ${MK6582SRC_DIR}/${basedir}/${filename}.apk
    echo "rename from $origfile to $filename" >> /tmp/list_random
}

function copyprocess() {
    echo > /tmp/list_random
    for subdir in $SUB_PROJECTS
    do
        echo "rename project apks $subdir"
        cd $subdir 

        echo "processing $subdir" >> /tmp/list_random
        apkfiles=$(find . -name *.apk -print | grep outputs | grep -v unsigned )
        for apkfile in $apkfiles
        do
            basename=$(echo $apkfile | cut -d '/' -f 2)
            case $basename in 
                kivviset)
                    copyfile KivviSet $apkfile $subdir
                    ;;
                kivvistore)
                    copyfile KivviStore $apkfile $subdir
                    ;;
                factorytest)
                    copyfile KivviFactoryTest $apkfile $subdir
                    ;;
                kivvimonitorservice)
                    copyfile KivviMonitorService $apkfile $subdir
                    ;;
                kivviguide)
                    copyfile KivviGuide $apkfile $subdir
                    ;;
                devicemanage)
                    copyfile KivviDeviceManage $apkfile $subdir
                    ;;
                mintkeydemo)
                    copyfile KivviMintKeyDemo $apkfile $subdir
                    ;;
                mintkeyservice)
                    copyfile KivviMintKeyService $apkfile $subdir
                    ;;
                kivvisystemsdkdemonew)
                    copyfile KivviSystemSdkDemoNew $apkfile $subdir
                    ;;
                kivvisystemservice)
                    copyfile KivviSystemService $apkfile $subdir
                    ;;
                app)
                    copyfile $subdir $apkfile $subdir
                    ;;
                *)
                    echo "error when rename and copy"
                    exit 1
                    ;;
            esac
        done
        cd ..
    done
}

mkdir -p $OUT_DIR
case $option in
    debug)
        cd $BASE_DIR
        for subdir in $SUB_PROJECTS
        do
            echo "build project $subdir"
            cd $subdir ; ./gradlew assembleDebug
            cd ..
        done

        copyprocess
        ;;
    release)
        cd $BASE_DIR
        for subdir in $SUB_PROJECTS
        do
            echo "build project $subdir"
            cd $subdir ; ./gradlew assembleRelease
            cd ..
        done

        copyprocess
        ;;
    clean)
        cd $BASE_DIR
        for subdir in $SUB_PROJECTS
        do
            echo "build project $subdir"
            cd $subdir ;  ./gradlew clean
            cd ..
        done
        rm -f ${OUT_DIR}/*.apk
        ;;
    install)
        cd $OUT_DIR
        installapks=$(ls -1)
        for installapk in $installapks
        do
            echo "install apk $installapk"
            adb install -r $installapk
            sleep 1
        done
        ;;
    *)
        echo "unknow command"
        ;;
esac



