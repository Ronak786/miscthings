#ifndef UTIL_H
#define UTIL_H

#include <SkCanvas.h>
#include <SkColor.h>
#include <SkPaint.h>
#include <SkPath.h>
#include <SkXfermode.h>
#include <android/native_window.h>

#include <utils/Log.h>  
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>

#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>
#include <gui/Surface.h>
#include <gui/ISurfaceComposer.h>  
#include <cutils/properties.h>
#include <cutils/memory.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>  
#include <ui/DisplayInfo.h>
#include <media/stagefright/foundation/ADebug.h>  
#include <android/native_window.h>  
#include <system/window.h>  
#include <ui/GraphicBufferMapper.h>  

#include <SkImageInfo.h>
#include <SkImageDecoder.h>
#include <SkGraphics.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkMatrix.h>
#include <SkDevice.h>
#include <SkStream.h>
#include "SkDevice.h"
#include "SkPaint.h"
#include "SkColorFilter.h"
#include "SkRect.h"
#include "SkTypeface.h"

//#define DEBUG 1
#ifdef	DEBUG
#define LOGV(fmt,...) printf(fmt,##__VA_ARGS__)
#else
#define LOGV(fmt,...) 
#endif

#define ALIGNWIDTH 16
enum ScreenType {
    MAIN,
    HDMI,
    SCREENMAX,
};

struct ScreenInfo {
    int width;
    int height;
    ScreenType type;
};

int getScreenInfo(ScreenInfo *info);
int fontWidth(char *text, int textSize);
void CreateBitmapRGBA8888(const char* showText, int tSize, int offset[], int offsz, SkColor fontcolor, int width, int height, unsigned char *rgba, bool horizontal);
int ALIGN(int x, int y);
void render(const void *data, size_t size, const android::sp<ANativeWindow> &nativeWindow, int width, int height);
android::sp<android::SurfaceControl>  prepareDisplayPanel(const char *name, int layer, ScreenInfo* info, int posX, int posY, int width, int height);
android::sp<android::SurfaceComposerClient> getComposerClient();
#endif
