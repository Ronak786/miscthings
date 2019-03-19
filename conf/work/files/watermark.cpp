/*********************
 *TODO: we have too many duplicate   Surface blablabla... calls,
 * should simplify

just one func: show on display(your text, your position),  watermark and displaymark use diff ones
so use diff display(func)
 ********************/
#include "watermark.h"
#include "util.h"

using namespace android;

static pthread_t tid; // each thread blend one text
static pthread_t fwtid;
static pthread_t hdtid;

static const char *propCustomer = "sys.kivvi.customer"; // the property to detect
static const char *customerFactory = "test";
static const char *customerDevelop = "develop";
static const char *develop_info_en= "Develop";

static const char *propFw = "sys.kivvi.security"; // the property to detect for flowing water
static const char *fwtag = "tampered";
static const char *fwText = "Device Tampered, Caution! ";

static const char *propHd = "sys.kivvi.hd"; // the property for hd
static const char *hdtag = "hd";
static const char *hdText = "We are showing! ";

static int textActivated = 0;
static int textChangeTo = 0;
static int needShowText = 0;
static int fwActivated = 0;
static int fwChangeTo = 0;
static int needShowFlowingWater = 0;
static int hdActivated = 0;
static int hdChangeTo = 0;
static int needShowHd = 0;
static void* DisplayText(void *para);
static void* DisplayFlowingWater(void *para);
static int showDebugMachineInfo();
static void sigThreadFunc(int sig);


int detectAndDrawWatermark()
{
    signal(SIGUSR2, sigThreadFunc);
    showDebugMachineInfo();
    return 0;
}

int showDebugMachineInfo()
{
    char valText[128], valFw[128], valHd[128];


    pthread_create(&tid, NULL, &DisplayText, (void*)develop_info_en);
  
    pthread_create(&fwtid, NULL, &DisplayFlowingWater, (void*)fwText);

    pthread_create(&hdtid, NULL, &DisplayFlowingWater, (void*)hdText);


    while(1) {
        memset(valText, '\0', sizeof(valText));
        property_get(propCustomer, valText, "none");
        if (!strcmp(valText, customerDevelop) || !strcmp(valText, customerFactory)) {
            textChangeTo = 1;
        } else {
            textChangeTo = 0;
        }

        property_get(propFw, valFw, "none");
        if (!strcmp(valFw, fwtag)) {
            fwChangeTo = 1;
        } else {
            fwChangeTo = 0;
        }

        property_get(propHd, valHd, "none");
        if (!strcmp(valHd, hdtag)) {
            hdChangeTo = 1;
        } else {
            hdChangeTo = 0;
        }

        // check for text show
        if (textActivated == 0 && textChangeTo == 1) { // set attr
            needShowText = 1;
            pthread_kill(tid, SIGUSR2);
            textActivated = 1;
        } else if (textActivated == 1 && textChangeTo == 0) { // unset attr
            needShowText = 0;
            pthread_kill(tid, SIGUSR2);
            textActivated = 0;
        }

        if (fwActivated == 0 && fwChangeTo == 1) { // set attr
            needShowFlowingWater = 1;
            pthread_kill(fwtid, SIGUSR2);
            fwActivated = 1;
        } else if (fwActivated == 1 && fwChangeTo == 0) { // unset attr
            needShowFlowingWater = 0;
            pthread_kill(fwtid, SIGUSR2);
            fwActivated = 0;
        }

        if (hdActivated == 0 && hdChangeTo == 1) { // set attr
            needShowHd = 1;
            pthread_kill(hdtid, SIGUSR2);
            hdActivated = 1;
        } else if (hdActivated == 1 && hdChangeTo == 0) { // unset attr
            needShowHd = 0;
            pthread_kill(hdtid, SIGUSR2);
            hdActivated = 0;
        }

        sleep(2); // this will wake each time signal is invoked
    }
	return 0;
}

void sigThreadFunc(int sig) {
    // use this to tell thread to wake up
}

static void* DisplayText(void *para){

    if (para == NULL) {
        return NULL;
    }

    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    int32_t textSize = 32;
    char text[32];
    int count;

    count = strlen((char*)para);
    memcpy(text,(char*)para,count);
    text[count] = '\0';

    int textWidth = ALIGN(fontWidth(text, textSize), ALIGNWIDTH);
    LOGV("textwidth is %d\n", textWidth);
    struct ScreenInfo info[SCREENMAX];
    getScreenInfo(info);

    int width = ALIGN(info[MAIN].width - ALIGNWIDTH, ALIGNWIDTH);
    int height = ALIGN(info[MAIN].height - ALIGNWIDTH, ALIGNWIDTH);

    sp<SurfaceControl> surfaceControl = prepareDisplayPanel("textDisplay", 0x40000000, 
                            &info[MAIN], -1, 0, width, height);
    if (surfaceControl == nullptr) {
        LOGV("can not get surfacecontrol");
    }

    sp<Surface> surface = surfaceControl->getSurface();

    int rgbaSize = width * height * 4;
    unsigned char* rgba = (unsigned char*)malloc(rgbaSize);
    if (rgba == nullptr) {
        LOGV("can not malloc in %s for rgba\n", __FUNCTION__);
        return nullptr;
    } 
    int offset[] = {textSize, textSize, width-textWidth, height-textSize, textSize, height-textSize, 
                    width-textWidth, textSize};
    CreateBitmapRGBA8888(text, textSize, offset, sizeof(offset)/sizeof(offset[0]),
                         SK_ColorRED, width, height, rgba, false);
    ANativeWindow_Buffer outBuffer;
    surface->lock(&outBuffer, NULL);
    render(rgba,rgbaSize,surface,width,height);  
    surfaceControl->setAlpha(1);

    while (1) {
        if (needShowText) {
            surfaceControl->setAlpha(1);
        } else {
            surfaceControl->setAlpha(0);
        }
        SurfaceComposerClient::closeGlobalTransaction();	
        sleep(100); // wake up by thread signal
        SurfaceComposerClient::openGlobalTransaction();	
    }

    LOGV("[%s][%d]\n",__FILE__,__LINE__);
    IPCThreadState::self()->joinThreadPool();
    IPCThreadState::self()->stopProcess();
    return NULL;
}

static void* DisplayFlowingWater(void *para){

    if (para == NULL) {
        return NULL;
    }

    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    bool horizontal = false;
    ScreenType sType = MAIN;

    int32_t textSize = 48;
    char text[32];
    int textCount = strlen((char*)para);
    memcpy(text,(char*)para, textCount);
    text[textCount] = '\0';

    if (!strcmp(text, hdText)) {
        horizontal = true;
        sType = HDMI;
    }

    int textWidth = ALIGN(fontWidth(text, textSize), ALIGNWIDTH);
    LOGV("textwidth is %d\n", textWidth);
    struct ScreenInfo info[SCREENMAX];
    getScreenInfo(info);

    int width, height;
    if (horizontal) {
        width = textSize  * 4; // realy text_height * 4
        height = ALIGN(info[sType].height -ALIGNWIDTH, ALIGNWIDTH); 
    } else {
        height = textSize  * 4; // realy text_height * 4
        width = ALIGN(info[sType].width -ALIGNWIDTH, ALIGNWIDTH); 
    }
    
    sp<SurfaceControl> surfaceControl = prepareDisplayPanel("textDisplay", 0x40000000, 
                            &info[sType], ALIGNWIDTH/2, textSize, width, height);
    if (surfaceControl == nullptr) {
        LOGV("can not get surfacecontrol");
    }

    sp<Surface> surface = surfaceControl->getSurface();

    ANativeWindow_Buffer outBuffer;
    surface->lock(&outBuffer, NULL);

    int rgbaSize = width * height * 4;
    unsigned char* rgba = (unsigned char*)malloc(rgbaSize);
    if (rgba == nullptr) {
        LOGV("can not malloc in %s for rgba\n", __FUNCTION__);
        return nullptr;
    } 

    int paintSize = horizontal ? height : width; 
    int paintStart = 0;
    while (1) {
        static int mainIndex = 0;
        int wrapIndex;
        int step = 4;
        while (horizontal ? needShowHd : needShowFlowingWater) {
            if (mainIndex >= paintSize) {
                mainIndex = paintStart;
            }
            surface->lock(&outBuffer, NULL);
            if (mainIndex <= paintStart + paintSize - textWidth) { // when not wrap
                int offset[] = {mainIndex, textSize};    
                CreateBitmapRGBA8888(text,textSize, offset,sizeof(offset)/sizeof(offset[0]),
                                SK_ColorYELLOW,width,height,rgba,horizontal);
                wrapIndex = -textWidth;
            } else {  // when wrap, we draw two text here
                int offset[] = {mainIndex, textSize, wrapIndex, textSize};    
                CreateBitmapRGBA8888(text,textSize, offset,sizeof(offset)/sizeof(offset[0]),
                                SK_ColorYELLOW,width,height,rgba,horizontal);
                wrapIndex += step;
            }
            render(rgba,rgbaSize,surface,width,height);  
            mainIndex += step;
            surfaceControl->setAlpha(1);
            SurfaceComposerClient::closeGlobalTransaction();	
            usleep(1000*200);
            SurfaceComposerClient::openGlobalTransaction();	
        } 
        mainIndex = paintStart;
        surfaceControl->setAlpha(0);
        SurfaceComposerClient::closeGlobalTransaction();	
        sleep(100);
        SurfaceComposerClient::openGlobalTransaction();	
//	surface->unlockAndPost();
    }

    LOGV("[%s][%d]\n",__FILE__,__LINE__);
    IPCThreadState::self()->joinThreadPool();
    IPCThreadState::self()->stopProcess();
    return NULL;
}
