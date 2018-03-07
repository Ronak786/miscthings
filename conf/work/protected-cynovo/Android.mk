# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_LDLIBS :=-llog
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
LOCAL_MODULE    := protected_cynovo 
LOCAL_SRC_FILES := main.cpp \
				serial_header.c \
				analysis_module.c \
				analysis_module_pangu.c \
				hlog.c \
				option.c \
				protect_cynovo.c \
				logo.c


LOCAL_MODULE_TAGS:= optional
LOCAL_PRELINK_MODULE := false
include ${BUILD_EXECUTABLE}
#BUILD_EXECUTABLE
#BUILD_SHARED_LIBRARY


