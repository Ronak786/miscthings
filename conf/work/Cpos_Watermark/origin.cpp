#include <SkCanvas.h>
#include <SkColor.h>
#include <SkPaint.h>
#include <SkXfermode.h>
#include <android/native_window.h>


#include <unistd.h>
#include <string.h>
#include <stdio.h>

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

static void* Displaychinese(void *para);

int ShowWaterMark(){
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
	ret = pthread_create(&tidT0, NULL, &Displaychinese, para);
	usleep(200*1000);

	para[0] = 1;
	ret = pthread_create(&tidT1, NULL, &Displaychinese, para);
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

static void* Displaychinese(void *para){
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

    // get image size
	SkFILEStream stream("/system/cpos.png");
	SkImageDecoder* codec = SkImageDecoder::Factory(&stream);
	if(!codec) return NULL;

    SkBitmap bmp;
    stream.rewind();
    codec->decode(&stream, &bmp, SkBitmap::kRGB_565_Config, SkImageDecoder::kDecodePixels_Mode);
    SkIRect bounds;
    bmp.getBounds(&bounds);
    uint32_t width = bounds.fRight;
    uint32_t height = bounds.fBottom;


	sp<SurfaceComposerClient> client = new SurfaceComposerClient();

    // get screen size
    DisplayInfo info;
    client->getDisplayInfo(client->connection(), &info);
    uint32_t sWitdh = info.w;
    uint32_t sHeight = info.h;
    
    // mury: surface size, size can be modified by setSize
	sp<SurfaceControl> surfaceControl = client->createSurface(String8("testsurface"),
		width, height, PIXEL_FORMAT_RGB_565, 0);
	sp<Surface> surface = surfaceControl->getSurface();

	SurfaceComposerClient::openGlobalTransaction();
	surfaceControl->setLayer(0x40000000);

    // mury: set transparency

    // mury: set surface pos
	if(((char*)para)[0] == 0)
		surfaceControl->setPosition(0, 40);
//		surfaceControl->setPosition(0 , sHeight - height);
	if(((char*)para)[0] == 1)
		surfaceControl->setPosition(0, 725);
	if(((char*)para)[0] == 2)
		surfaceControl->setPosition(1180, 40);
	if(((char*)para)[0] == 3)
		surfaceControl->setPosition(1180, 725);

    // mury: set surface size (not image size
//	surfaceControl->setSize(500, 280);
	SurfaceComposerClient::closeGlobalTransaction();	

	ANativeWindow_Buffer outBuffer;
	surface->lock(&outBuffer, NULL);
	ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);
	android_memset16((uint16_t*)outBuffer.bits, 0xF800, bpr*outBuffer.height);
	surface->unlockAndPost();

	surface->lock(&outBuffer, NULL);
    
	if(((char*)para)[0] == 0) {
        // mury: set pic size
        surfaceControl->setAlpha(.5);
        SkBitmap bitmap;
        bitmap.setConfig(convertPixelFormat(outBuffer.format), width, height, bpr);
        bitmap.setPixels(outBuffer.bits);
        SkCanvas canvas(bitmap);
        canvas.drawBitmap(bmp, SkIntToScalar(0), SkIntToScalar(0));
    }

	if(((char*)para)[0] == 1) {
        surfaceControl->setAlpha(0);
        char text[50];
        SkBitmap bitmap;
        bitmap.setConfig(convertPixelFormat(outBuffer.format), width, height, bpr);
        bitmap.setPixels(outBuffer.bits);
        SkCanvas canvas(bitmap);
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setTextSize(24);
        paint.setColor(SkColorSetRGB(255,0,0)); // red
        paint.setStyle(SkPaint::kStroke_Style);  // style of text
        canvas.clear(SK_ColorWHITE);
//        canvas.clear(SkColorSetARGB(0,255,255,255));
        int count = sprintf(text, "height %ld, width %ld", client->getHeight(), client->getWidth());
        text[count] = '\0';
        canvas.drawText(text, strlen(text), 0, height/2.0, paint );
    }

	surface->unlockAndPost();
	printf("[%s][%d]\n",__FILE__,__LINE__);
	IPCThreadState::self()->joinThreadPool();
	IPCThreadState::self()->stopProcess();
	return NULL;
}
