#include "util.h"

using namespace android;

// check null in case of fail
// single instance for process
sp<SurfaceComposerClient> getComposerClient() {
    static sp<SurfaceComposerClient> client = nullptr;
    if (client == nullptr) {
        client = new SurfaceComposerClient();
    }
    return client;
}

int getScreenInfo(struct ScreenInfo *info) {
    sp<IBinder> dtokenMain(SurfaceComposerClient::getBuiltInDisplay(  
            ISurfaceComposer::eDisplayIdMain));  
    //获取main屏幕的宽高等信息  
    DisplayInfo dinfo;
    status_t status = SurfaceComposerClient::getDisplayInfo(dtokenMain, &dinfo);  
    if (!status) {
        LOGV("Main w=%d,h=%d,xdpi=%f,ydpi=%f,fps=%f,ds=%f\n",   
            dinfo.w, dinfo.h, dinfo.xdpi, dinfo.ydpi, dinfo.fps, dinfo.density);  
        info[MAIN].width = dinfo.w;
        info[MAIN].height = dinfo.h;
	info[MAIN].type = MAIN;
    }


    //获取hdmi屏幕的宽高等信息  
    sp<IBinder> dtokenHdmi(SurfaceComposerClient::getBuiltInDisplay(  
            ISurfaceComposer::eDisplayIdHdmi));  
    status = SurfaceComposerClient::getDisplayInfo(dtokenHdmi, &dinfo);  
    if (!status) {
        LOGV("Hdmi w=%d,h=%d,xdpi=%f,ydpi=%f,fps=%f,ds=%f\n",   
            dinfo.w, dinfo.h, dinfo.xdpi, dinfo.ydpi, dinfo.fps, dinfo.density);  
        info[HDMI].width = dinfo.w;
        info[HDMI].height = dinfo.h;
	info[HDMI].type = HDMI;
    }
    return 0;
}

// get fonts width for drawing
int fontWidth(char *text, int textSize) {
    int textCount = strlen(text);
    text[textCount] = '\0';

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setTextSize(textSize);
    paint.setColor(SkColorSetRGB(0,255,0)); // yellow
    paint.setStyle(SkPaint::kFill_Style);  // style of text

    // get text length on screen
    int glyphCount = paint.getTextWidths(text, textCount, NULL);
    SkScalar widths[glyphCount];
    paint.getTextWidths(text, glyphCount, widths);
    int textWidth = 0;
    for (int i = 0; i < glyphCount; ++i) {
        textWidth += widths[i];
    }
	textWidth = ALIGN(textWidth, 16);
    return textWidth;
}

#if 0
// debug functions
static void dump(unsigned char*buf, int siz) {
    for (int i = 0; i < siz; ++i) {
        printf("%02x ", buf[i]);
        if (i % 64 == 63) {
            putchar('\n');
        }
    }
}

static void argb2rgba(unsigned char* buf, int siz) {
    char ch = '\0';
    char a;
    for (int i = 0; i < siz; i+=2) {
        buf[i] = 0xf;
        buf[i+1] = 0xf8;
    }
    printf("after %d\n", siz);
}
#endif

void CreateBitmapRGBA8888(const char* showText, int tSize, int offset[], int offsz, SkColor fontcolor, int width, int height, unsigned char *rgba, bool horizontal){
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setTextSize(tSize);
    paint.setStyle(SkPaint::kFill_Style);  // style of text

    SkBitmap bitmap;
    bitmap.setInfo(SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kOpaque_SkAlphaType),0);
//    bitmap.setInfo(SkImageInfo::Make(width, height, kARGB_4444_SkColorType, kOpaque_SkAlphaType),0);
    bitmap.allocPixels();

    LOGV("width: %d  height: %d\n", bitmap.width(), bitmap.height());
    LOGV("empty: %s\n", bitmap.empty() ? "true" : "false");
    LOGV("bitmap does %shave pixels\n", bitmap.isNull() ? "not " : "");
    LOGV("bitmap has draw %s\n", bitmap.drawsNothing() ? "nothing " : "something");

    SkCanvas canvas(bitmap);
    canvas.clear(0x00000000);//背景颜色
    paint.setColor(fontcolor);//字体颜色

    if (horizontal) {
        for (int i = 0; i < offsz; i+=2) {
            SkPath path;
            path.moveTo(SkIntToScalar(offset[i+1]), SkIntToScalar(height - offset[i]));
            path.lineTo(SkIntToScalar(offset[i+1]), SkIntToScalar(0));

            canvas.drawTextOnPath(showText, strlen(showText), path, nullptr, paint);
        }
    } else {
        for (int i = 0; i < offsz; i+=2) {
            canvas.drawText(showText, strlen(showText), offset[i], offset[i+1], paint);
        }
    }
    bitmap.lockPixels();
    unsigned char* pixels = (unsigned char*)bitmap.getPixels();
    int pixels_size = bitmap.getSize();
    memcpy(rgba, pixels, pixels_size);
    bitmap.unlockPixels();
//    argb2rgba(rgba, pixels_size);
}

int ALIGN(int x, int y) {  
    return (x + y - 1) & ~(y - 1);  
}  

void render(const void *data, size_t size, const sp<ANativeWindow> &nativeWindow, int width, int height) {  
    LOGV("begin render\n");
    sp<ANativeWindow> mNativeWindow = nativeWindow;  
    int err;  
    int mCropWidth = width;  
    int mCropHeight = height;  
      
    int halFormat = PIXEL_FORMAT_RGBA_8888;//颜色空间  
    int bufWidth = (mCropWidth + 1) & ~1;//按2对齐  
    int bufHeight = (mCropHeight + 1) & ~1;  
      
    CHECK_EQ(0,  
            native_window_set_usage(  
            mNativeWindow.get(),  
            GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN  
            | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP));  
  
    CHECK_EQ(0,  
            native_window_set_scaling_mode(  
            mNativeWindow.get(),  
            NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW));  
  
    // Width must be multiple of 32???  
    //很重要,配置宽高和和指定颜色空间yuv420  
    //如果这里不配置好，下面deque_buffer只能去申请一个默认宽高的图形缓冲区  
    CHECK_EQ(0, native_window_set_buffers_geometry(  
                mNativeWindow.get(),  
                bufWidth,  
                bufHeight,  
                halFormat));  
      
      
    ANativeWindowBuffer *buf;//描述buffer  
    //申请一块空闲的图形缓冲区  
    if ((err = native_window_dequeue_buffer_and_wait(mNativeWindow.get(),  
            &buf)) != 0) {  
        ALOGW("Surface::dequeueBuffer returned error %d", err);  
        return;  
    }  
  
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();  
  
    Rect bounds(mCropWidth, mCropHeight);  
  
    void *dst;  
    CHECK_EQ(0, mapper.lock(//用来锁定一个图形缓冲区并将缓冲区映射到用户进程  
                buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));//dst就指向图形缓冲区首地址  
    memcpy(dst, data, width * height * 4);
  
    CHECK_EQ(0, mapper.unlock(buf->handle));  
  
    if ((err = mNativeWindow->queueBuffer(mNativeWindow.get(), buf,  
            -1)) != 0) {  
        ALOGW("Surface::queueBuffer returned error %d", err);  
    }  
    buf = NULL;  
}  

// specific display strategy will be handled in specific file, like flowing or just different places
sp<SurfaceControl>  prepareDisplayPanel(const char *name, int layer, ScreenInfo* info, int posX, int posY, int width, int height) {
    sp<SurfaceComposerClient>  client = getComposerClient();
    if (client == nullptr) {
        return nullptr;
    }
    sp<SurfaceControl> surfaceControl = client->createSurface(String8(name),  
            info->width, info->height, PIXEL_FORMAT_RGBA_8888, 0);  

    SurfaceComposerClient::openGlobalTransaction();
    surfaceControl->setLayer(layer);
    surfaceControl->setPosition(posX, posY);
    surfaceControl->setSize(width, height);
    surfaceControl->setLayerStack(info->type);
    SurfaceComposerClient::closeGlobalTransaction();	
    return surfaceControl;
}
