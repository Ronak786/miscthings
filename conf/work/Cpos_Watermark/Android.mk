LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#APP_ALLOW_MISSING_DEPS := true
#LOCAL_PRELINK_MODULE := false 
#LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_SRC_FILES:= \
	Watermark.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libandroidfw \
	libutils \
	libhardware \
	libhardware_legacy \
	libskia \
	libgui \
	libui \
	libbinder

#LOCAL_LDLIBS := -lcutils -llog -landroidfw -lutils -lhardware -l hardware_legacy -lskia -lgui -lui -lbinder

LOCAL_CFLAGS += -pie -fPIE 
LOCAL_LDFLAGS += -pie -fPIE

MY_LOCAL_ANDSRC := /home/sora/gitbase/pangu_mt6582
LOCAL_C_INCLUDES := \
	hardware/libhardware/include/ \
	frameworks/native/include/ \
	system/core/include \
	external/skia/include/core \
	external/skia/include/lazy \
	external/skia/include/images \
	frameworks/native/include \
	external/kernel-headers/original/uapi

LOCAL_MODULE:= Cpos_Watermark

include $(BUILD_EXECUTABLE)
