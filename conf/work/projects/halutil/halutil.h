/*************************************************************************
	> File Name: halutil.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 11:03:01 AM CST
 ************************************************************************/


#ifndef HALUTIL_H
#define HALUTIL_H
#include "wifiinfo.h"
#include "camerainfo.h"
#include "lteinfo.h"
#include <vector>


class HalUtil {
public:
	static CameraInfo* openCamera();
	static int closeCamera(CameraInfo *info);

	static LteInfo* openLte();
	static int closeLte(LteInfo *info);

	// get wifiinfo list and return, static method
	static std::vector<WifiInfo>  getAvailWifiList();
};
#endif
