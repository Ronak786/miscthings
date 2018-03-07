
#include "option.h"
#include "analysis_module.h"
#include "analysis_module_pangu.h"
#include "serial_header.h"
#include "hlog.h"
#include "watermark.h"

using namespace android;

struct ImageInfo {
    SkBitmap bmp;
    int32_t height;
    int32_t width;
};

const char* develop_info_en= "Develop";
static pthread_t tids[4] = {0}; // each thread blend one text
static pthread_t imageTid = 0;
static pthread_t imageTextTid = 0;
const char *propCustomer = "sys.kivvi.customer"; // the property to detect
const char *propSecurity = "sys.kivvi.security"; // the property to detect
const char *securityTamper = "tampered";
const char *customerFactory = "factory";
const char *customerDevelop = "develop";
int textActivated = 0;
int textChangeTo = 0;
int needShowText = 0;
int imageActivated = 0;
int imageChangeTo = 0;
int needShowImage = 0;

static void* DisplayText(void *para);
static void* DisplayImage(void *para);
static void* DisplayMiddleText(void *para);
int showDebugMachineInfo();
int showTempWarning(int ac, char *av[]);
int detectAndDrawWatermark();
void sendSignal();
void sigThreadFunc(int sig);
static int ShowWaterMark();

static SkColorType convertPixelFormat(PixelFormat format) {
    /* note: if PIXEL_FORMAT_RGBX_8888 means that all alpha bytes are 0xFF, then
        we can map to SkBitmap::kARGB_8888_Config, and optionally call
        bitmap.setIsOpaque(true) on the resulting SkBitmap (as an accelerator)
    */
    switch (format) {
    case PIXEL_FORMAT_RGBX_8888:    return kRGBA_8888_SkColorType;
    case PIXEL_FORMAT_RGBA_8888:    return kRGBA_8888_SkColorType;
    case PIXEL_FORMAT_RGBA_4444:    return kARGB_4444_SkColorType;
    case PIXEL_FORMAT_RGB_565:      return kRGB_565_SkColorType;
    default:                        return kUnknown_SkColorType;
    }   
}

int daemon()
{
	if(fork() != 0)
		exit(1);
	if(fork() != 0)
		exit(1);
	return 0;
}

int main(int argc, char **argv)
{
	init_serial_header();

	analysis_module_pangu.socket_type = sockettype;
	strncpy(analysis_module_pangu.socket_addr, socketaddr, sizeof(analysis_module_pangu.socket_addr));
	analysis_module_init(&analysis_module_pangu);

    detectAndDrawWatermark();

	while(1)
		sleep(1);	

	return 0;
}

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

// this part executed when program is called with argument "kill"
int detectAndDrawWatermark()
{
     
    signal(SIGUSR2, sigThreadFunc);
    showDebugMachineInfo(); // means show text
	return 0;
}

int showDebugMachineInfo()
{
	char para[2] = {0};
	para[0] = 0;
    char valText[128], valImage[128];

    while(1) {
        property_get(propCustomer, valText, "none");
        if (!strncmp(valText, customerDevelop, strlen(customerDevelop)) 
                || !strncmp(valText, customerFactory, strlen(customerFactory))) {
            textChangeTo = 1;
        } else {
            textChangeTo = 0;
        }
        property_get(propSecurity, valImage, "none");
        if (!strncmp(valImage, securityTamper, strlen(securityTamper))) {
            imageChangeTo = 1;
        } else {
            imageChangeTo = 0;
        }

        if (textActivated == 0 && textChangeTo == 1) { // set attr
            needShowText = 1;
            if (tids[0] == 0) { // not start thread yet
                for (size_t i = 0; i < sizeof(tids)/sizeof(tids[0]); ++i) {
                    para[0] = i;
                    pthread_create(&tids[i], NULL, &DisplayText, para);
                    usleep(200*1000);
                }
            }
            for (size_t i = 0; i < sizeof(tids)/sizeof(tids[0]); ++i) {
                pthread_kill(tids[i], SIGUSR2);
                usleep(200*1000);
            }
            textActivated = 1;
        } else if (textActivated == 1 && textChangeTo == 0) { // unset attr
            // should add pthread create in case of unconcerned problem?
            needShowText = 0;
            for (size_t i = 0; i < sizeof(tids)/sizeof(tids[0]); ++i) {
                pthread_kill(tids[i], SIGUSR2);
                usleep(200*1000);
            }
            textActivated = 0;
        }

        //check for image
        if (imageActivated == 0 && imageChangeTo == 1) { // set attr
            needShowImage = 1;
            if (imageTid == 0) { // not start thread yet
                para[0] = 0;
                pthread_create(&imageTid, NULL, &DisplayImage, para);
                usleep(200*1000);
                pthread_create(&imageTextTid, NULL, &DisplayMiddleText, para);
                usleep(200*1000);
            }
            pthread_kill(imageTid, SIGUSR2);
            usleep(200*1000);
            pthread_kill(imageTextTid, SIGUSR2);
            imageActivated = 1;
        } else if (imageActivated == 1 && imageChangeTo == 0) { // unset attr
            // should add pthread create in case of unconcerned problem?
            needShowImage = 0;
            pthread_kill(imageTid, SIGUSR2);
            usleep(200*1000);
            pthread_kill(imageTextTid, SIGUSR2);
            usleep(200*1000);
            imageActivated = 0;
        }
        sleep(2); // this will wake each time signal is invoked
    }
	return 0;
}

void sigThreadFunc(int sig) {
    // use this to tell thread to wake up
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
    codec->decode(&stream, &info->bmp, kRGB_565_SkColorType, SkImageDecoder::kDecodePixels_Mode);
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
//    bitmap.setConfig(convertPixelFormat(buffer.format), iInfo.width, iInfo.height, ibpr);
    SkImageInfo imageInfo = SkImageInfo::Make(iInfo.width, iInfo.height, convertPixelFormat(buffer.format), kOpaque_SkAlphaType);
    bitmap.setInfo(imageInfo, ibpr);
    bitmap.setPixels(buffer.bits);
    SkCanvas canvas(bitmap);
    // draw partition offset of picture
    canvas.drawBitmap(iInfo.bmp, picX, picY);
}

static void* DisplayText(void *para){
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

	sp<SurfaceComposerClient> client = new SurfaceComposerClient();

    int32_t textSize = 32;
    char text[32];
    int count;

	count = strlen(develop_info_en);
	memcpy(text,develop_info_en,count);
    text[count] = '\0';

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setTextSize(textSize);
    paint.setColor(SkColorSetRGB(255,0,0)); // red
    paint.setStyle(SkPaint::kStroke_Style);  // style of text

    // get text length on screen
    int glyphCount = paint.getTextWidths(text, count, NULL);
    SkScalar widths[glyphCount];
    paint.getTextWidths(text, glyphCount, widths);
    int textWidth = 0;
    for (int i = 0; i < glyphCount; ++i) {
        textWidth += widths[i];
    }

    // mury: surface size, size can be modified by setSize
    sp<SurfaceControl> surfaceControl = client->createSurface(String8("testsurface"),
            textWidth, textSize, PIXEL_FORMAT_RGB_565, 0);
	sp<Surface> surface = surfaceControl->getSurface();

	SurfaceComposerClient::openGlobalTransaction();
	surfaceControl->setLayer(0x40000000);
    if (((char*)para)[0] == 0){
        surfaceControl->setPosition(0,50);
    } else if (((char*)para)[0] == 1) {
        surfaceControl->setPosition(720-textWidth, 1280-textSize-96);
    } else if(((char*)para)[0] == 2) {
        surfaceControl->setPosition(0, 1280-textSize-96);
    } else if(((char*)para)[0] == 3){
        surfaceControl->setPosition(720-textWidth, 50);
    } else { // display in the middle along with image
        surfaceControl->setPosition((720-textWidth)/2, 1280/2 + 90);
    }

    // mury: set surface size , this function have no effect !!!
	ANativeWindow_Buffer outBuffer;
	surface->lock(&outBuffer, NULL);
	ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);
	android_memset16((uint16_t*)outBuffer.bits, 0xF800, bpr*outBuffer.height);
	surface->unlockAndPost();
	surface->lock(&outBuffer, NULL);
    
	surfaceControl->setAlpha(0.5);
	SkBitmap bitmap;
//	bitmap.setConfig(convertPixelFormat(outBuffer.format), textWidth, textSize, bpr);
    SkImageInfo imageInfo = SkImageInfo::Make(textWidth, textSize, convertPixelFormat(outBuffer.format), kOpaque_SkAlphaType);
    bitmap.setInfo(imageInfo, bpr);
	bitmap.setPixels(outBuffer.bits);
	SkCanvas canvas(bitmap);
	canvas.clear(SK_ColorWHITE);
	canvas.drawText(text, textWidth, 0, textSize, paint);
	surface->unlockAndPost();
	SurfaceComposerClient::closeGlobalTransaction();	

    while (1) {
        if (needShowText) {
            surfaceControl->setAlpha(0.5);
        } else {
            surfaceControl->setAlpha(0);
        }
        SurfaceComposerClient::closeGlobalTransaction();	
        sleep(100); // wake up by thread signal
        SurfaceComposerClient::openGlobalTransaction();	
    }

	printf("[%s][%d]\n",__FILE__,__LINE__);
	IPCThreadState::self()->joinThreadPool();
	IPCThreadState::self()->stopProcess();
	return NULL;
}

static void* DisplayMiddleText(void *para){
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

	sp<SurfaceComposerClient> client = new SurfaceComposerClient();

    int32_t textSize = 42;
    char text[] = "TAMPERED";
    int count = strlen(text);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setTextSize(textSize);
    paint.setColor(SkColorSetRGB(255,0,0)); // red
    paint.setStyle(SkPaint::kStroke_Style);  // style of text

    // get text length on screen
    int glyphCount = paint.getTextWidths(text, count, NULL);
    SkScalar widths[glyphCount];
    paint.getTextWidths(text, glyphCount, widths);
    int textWidth = 0;
    for (int i = 0; i < glyphCount; ++i) {
        textWidth += widths[i];
    }
    
    // mury: surface size, size can be modified by setSize
    sp<SurfaceControl> surfaceControl = client->createSurface(String8("testsurface"),
            textWidth, textSize, PIXEL_FORMAT_RGB_565, 0);
	sp<Surface> surface = surfaceControl->getSurface();

	SurfaceComposerClient::openGlobalTransaction();
	surfaceControl->setLayer(0x40000000);
    surfaceControl->setPosition((720-textWidth)/2, 1280/2 + 150);

	ANativeWindow_Buffer outBuffer;
	surface->lock(&outBuffer, NULL);
	ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);
	android_memset16((uint16_t*)outBuffer.bits, 0xF800, bpr*outBuffer.height);
	surface->unlockAndPost();
	surface->lock(&outBuffer, NULL);
    
	surfaceControl->setAlpha(0.5);
	SkBitmap bitmap;
//	bitmap.setConfig(convertPixelFormat(outBuffer.format), textWidth, textSize, bpr);
    SkImageInfo imageInfo = SkImageInfo::Make(textWidth, textSize, convertPixelFormat(outBuffer.format), kOpaque_SkAlphaType);
    bitmap.setInfo(imageInfo, bpr);
	bitmap.setPixels(outBuffer.bits);
	SkCanvas canvas(bitmap);

    canvas.clear(SK_ColorYELLOW);
	canvas.drawText(text, textWidth, 0, textSize, paint);
	surface->unlockAndPost();

    while (1) {
        if (needShowImage) {
            surfaceControl->setAlpha(.5);
        } else {
            surfaceControl->setAlpha(0);
        }
        SurfaceComposerClient::closeGlobalTransaction();	
        sleep(100); // wake up by thread signal
    }

	printf("[%s][%d]\n",__FILE__,__LINE__);
	IPCThreadState::self()->joinThreadPool();
	IPCThreadState::self()->stopProcess();
	return NULL;
}

static void* DisplayImage(void *para) {
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

    ImageInfo imageInfo;
    if (getImageInfo("/system/etc/tampered.png", &imageInfo) == -1) return NULL;

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


	surfaceControl->setPosition((720-imageInfo.width)/2, (1280-imageInfo.height)/2);

	ANativeWindow_Buffer outBuffer;
	surface->lock(&outBuffer, NULL);
	ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);
	android_memset16((uint16_t*)outBuffer.bits, 0xF800, bpr*outBuffer.height);
	surface->unlockAndPost();
	surface->lock(&outBuffer, NULL);
    
	putOnImage(0.5, surfaceControl, imageInfo, outBuffer, 0, 0, bpr);

	surface->unlockAndPost();
	SurfaceComposerClient::closeGlobalTransaction();	
    while (1) {
        if (needShowImage) {
            surfaceControl->setAlpha(.5);
        } else {
            surfaceControl->setAlpha(0);
        }
        SurfaceComposerClient::closeGlobalTransaction();	
        sleep(100); // wake up by thread signal
        SurfaceComposerClient::openGlobalTransaction();	
    }
	printf("[%s][%d]\n",__FILE__,__LINE__);
	IPCThreadState::self()->joinThreadPool();
	IPCThreadState::self()->stopProcess();
	return NULL;
}
