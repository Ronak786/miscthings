#include <SkCanvas.h>
#include <SkColor.h>
#include <SkPaint.h>
#include <SkXfermode.h>
#include <android/native_window.h>


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>

#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>
#include <gui/Surface.h>
#include <cutils/properties.h>
#include <cutils/memory.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <ui/DisplayInfo.h>
//#include <ui/Rect.h>
//#include <ui/Region.h>

#include <SkImageDecoder.h>
#include <SkGraphics.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkDevice.h>
#include <SkStream.h>
#include "SkBitmap.h"
#include "SkDevice.h"
#include "SkPaint.h"
#include "SkColorFilter.h"
#include "SkRect.h"
//#include "SkImageEncoder.h" // not found
#include "SkTypeface.h"
//#include <hardware/hwcomposer_defs.h>

//#include <core/SkImageDecoder.h>
//#include <cutils/properties.h>
using namespace android;

struct ImageInfo {
    SkBitmap bmp;
    int32_t height;
    int32_t width;
};

static SkBitmap::Config convertPixelFormat(PixelFormat format) {
    /* note: if PIXEL_FORMAT_RGBX_8888 means that all alpha bytes are 0xFF, then
        we can map to SkBitmap::kARGB_8888_Config, and optionally call
        bitmap.setIsOpaque(true) on the resulting SkBitmap (as an accelerator)
    */
    switch (format) {
    case PIXEL_FORMAT_RGBX_8888:    return SkBitmap::kARGB_8888_Config;
    case PIXEL_FORMAT_RGBA_8888:    return SkBitmap::kARGB_8888_Config;
    case PIXEL_FORMAT_RGBA_4444:    return SkBitmap::kARGB_4444_Config;
    case PIXEL_FORMAT_RGB_565:      return SkBitmap::kRGB_565_Config;
    default:                        return SkBitmap::kNo_Config;
    }   
}

static void* DisplayText(void *para);
static void* DisplayImage(void *para);

static int ShowWaterMark(){
	char watermarksetting[PROPERTY_VALUE_MAX] = {0};
	memset(watermarksetting,0,sizeof(watermarksetting));	

	property_get("sys.cpos.watermark", watermarksetting, "show");

	if(watermarksetting == NULL)
		return -1;
	else if(strncmp(watermarksetting,"show",strlen("show")) == 0)
		return 0;
	else
		return 1;

	return 1;
}

int main(int argc, char** argv)
{
	int ret = 0;
	int position = 0;
    char para[2] = {0};
	para[0] = 0;
	pthread_t tidT0,tidT1,tidT2,tidT3;
	
	if(ShowWaterMark()!=0)
		return -1;

	para[0] = 0;
	ret = pthread_create(&tidT0, NULL, &DisplayImage, para);
	usleep(200*1000);

	para[0] = 1;
	ret = pthread_create(&tidT1, NULL, &DisplayText, para);
	usleep(200*1000);

    /*
	para[0] = 2;
	ret = pthread_create(&tidT2, NULL, &Displaychinese, para);
	usleep(200*1000);

	para[0] = 3;
	ret = pthread_create(&tidT3, NULL, &Displaychinese, para);
    */
	while(1);
	return 0;
}

static void getScreenSize(int32_t *width, int32_t *height) {
    struct fb_var_screeninfo fb_var;
    int fd = open("/dev/graphics/fb0", O_RDONLY);
    ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
    close(fd);
    *height = fb_var.xres;
    *width = fb_var.yres;
}

static int getImageInfo(const char* path, ImageInfo *info) {
    // get image size
	SkFILEStream stream(path);
	SkImageDecoder* codec = SkImageDecoder::Factory(&stream);
	if(!codec) return -1;

    stream.rewind();
    codec->decode(&stream, &info->bmp, SkBitmap::kRGB_565_Config, SkImageDecoder::kDecodePixels_Mode);
    SkIRect bounds;
    info->bmp.getBounds(&bounds);
    info->width = bounds.fRight;
    info->height = bounds.fBottom;
    return 0;
}

void putOnImage(float transparent, sp<SurfaceControl> &sControl, 
            ImageInfo &iInfo, ANativeWindow_Buffer &buffer, 
            int32_t picX, int32_t picY, ssize_t ibpr) {
    sControl->setAlpha(transparent);
    SkBitmap bitmap;
    bitmap.setConfig(convertPixelFormat(buffer.format), iInfo.width, iInfo.height, ibpr);
    bitmap.setPixels(buffer.bits);
    SkCanvas canvas(bitmap);
    // draw partition offset of picture
    canvas.drawBitmap(iInfo.bmp, picX, picY);
}

static void* DisplayText(void *para){
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

	sp<SurfaceComposerClient> client = new SurfaceComposerClient();

    int32_t sHeight, sWidth;
    getScreenSize(&sWidth, &sHeight);

    // create string and set paint
    int32_t textSize = 24;
    char text[50];
    int count = sprintf(text, "height %d, width %d", sHeight, sWidth);
    text[count] = '\0';
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setTextSize(textSize);
    paint.setColor(SkColorSetRGB(255,0,0)); // red
    paint.setStyle(SkPaint::kStroke_Style);  // style of text

    // get text length on screen
    int glyphCount = paint.getTextWidths(text, strlen(text), NULL);
    SkScalar widths[count];
    paint.getTextWidths(text, glyphCount, widths);
    int textWidth = 0;
    for (int i = 0; i < glyphCount; ++i) {
        textWidth += widths[i];
    }
    
    // mury: surface size, size can be modified by setSize
    // this is hard restriction
    sp<SurfaceControl> surfaceControl = client->createSurface(String8("testsurface"),
            textWidth, textSize, PIXEL_FORMAT_RGB_565, 0);
	sp<Surface> surface = surfaceControl->getSurface();

	SurfaceComposerClient::openGlobalTransaction();
	surfaceControl->setLayer(0x40000000);
    surfaceControl->setPosition(720-360-10, 1280-360-10);

    // mury: set surface size , this function have no effect !!!
//	surfaceControl->setSize(500, 280);  
	SurfaceComposerClient::closeGlobalTransaction();	

	ANativeWindow_Buffer outBuffer;
	surface->lock(&outBuffer, NULL);
	ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);
	android_memset16((uint16_t*)outBuffer.bits, 0xF800, bpr*outBuffer.height);
	surface->unlockAndPost();
	surface->lock(&outBuffer, NULL);
    
    surfaceControl->setAlpha(0);
    SkBitmap bitmap;
    bitmap.setConfig(convertPixelFormat(outBuffer.format), textWidth, textSize, bpr);
    bitmap.setPixels(outBuffer.bits);
    SkCanvas canvas(bitmap);
    canvas.clear(SK_ColorWHITE);

//        canvas.clear(SkColorSetARGB(0,255,255,255));
    canvas.drawText(text, strlen(text), 0, textSize, paint );

	surface->unlockAndPost();
	printf("[%s][%d]\n",__FILE__,__LINE__);
	IPCThreadState::self()->joinThreadPool();
	IPCThreadState::self()->stopProcess();
	return NULL;
}

static void* DisplayImage(void *para) {
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

    ImageInfo imageInfo;
    if (getImageInfo("/system/cpos.png", &imageInfo) == -1) return NULL;

	sp<SurfaceComposerClient> client = new SurfaceComposerClient();

    // write in one func
    int32_t sHeight, sWidth;
    getScreenSize(&sWidth, &sHeight);

    // mury: surface size, size can be modified by setSize
    // this is hard restriction
    sp<SurfaceControl> surfaceControl = client->createSurface(String8("testsurface"),
            imageInfo.width, imageInfo.height, PIXEL_FORMAT_RGB_565, 0);
	sp<Surface> surface = surfaceControl->getSurface();

	SurfaceComposerClient::openGlobalTransaction();
	surfaceControl->setLayer(0x40000000);
    // mury: set surface pos
    surfaceControl->setPosition(0, 40);

	SurfaceComposerClient::closeGlobalTransaction();	

	ANativeWindow_Buffer outBuffer;
	surface->lock(&outBuffer, NULL);
	ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);
	android_memset16((uint16_t*)outBuffer.bits, 0xF800, bpr*outBuffer.height);
	surface->unlockAndPost();
	surface->lock(&outBuffer, NULL);
    
    putOnImage(0.5, surfaceControl, imageInfo, outBuffer, 0, 0, bpr);

	surface->unlockAndPost();
	printf("[%s][%d]\n",__FILE__,__LINE__);
	IPCThreadState::self()->joinThreadPool();
	IPCThreadState::self()->stopProcess();
	return NULL;
}
