#ifndef _WATERMARK_H
#define _WATERMARK_H

#include <SkCanvas.h>
#include <SkColor.h>
#include <SkPaint.h>
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

int detectAndDrawWatermark();

#endif
