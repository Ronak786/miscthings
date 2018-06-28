/*************************************************************************
	> File Name: main.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 10:44:53 AM CST
 ************************************************************************/

#include "halutil.h"
#include <cstdio>

int main() {

	/*********************wifi***************************/
	std::vector<WifiInfo> vinfo = HalUtil::getAvailWifiList();
	for (auto wifiinfo: vinfo) {
		printf("vinfo: ;%s; ;%d; ;%s;\n", wifiinfo.getQuality().c_str(), wifiinfo.getLevel(), wifiinfo.getEssid().c_str());
	}
	vinfo[0].activate(std::string("Cyn0v0123")); // passin password
	std::string wifiinfo = vinfo[0].getIp();
	std::printf("wifi ip addr is %s\n", wifiinfo.c_str());
	vinfo[0].deactivate();			// deactivate
	/*********************wifi***************************/

	/*********************camera***************************/
	CameraInfo *cptr = HalUtil::openCamera();
	cv::Mat matpic;
	int res = cptr->getImage(matpic);
	std::printf("success get image\n");
	// show image ....
	HalUtil::closeCamera(cptr);
	/*********************camera***************************/

	/*********************lte***************************/
	LteInfo *lptr = HalUtil::openLte();
	std::string lteIp = lptr->getIp();
	int lteSignalStrength = lptr->getSignalStrength();
	std::printf("lte ip is %s, sig strength is %d\n", lteIp.c_str(), lteSignalStrength);

	HalUtil::closeLte(lptr);
	/*********************lte***************************/

	return 0;
}
