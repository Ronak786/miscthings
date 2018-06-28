/*************************************************************************
	> File Name: halutil.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 11:04:19 AM CST
 ************************************************************************/

#include <cstdio>

#include "halutil.h"

std::vector<WifiInfo> HalUtil::getAvailWifiList() {
	std::vector<WifiInfo> vinfo;
	std::string cmdstr = "iwlist wlp3s0 scanning | egrep '(ESSID|Signal level=)' | sed  'N;s/\\n/ /'";
	std::FILE* fp = popen(cmdstr.c_str(), "r");
	if (!fp) {
		printf("read pipe error\n");
		return vinfo;
	}

	// used to store result line
	char buf[256];
	int count;

	char quality[20], essid[128];
	int level;
	while (std::fgets(buf, sizeof(buf)-1, fp) != NULL) {
		if ((count = std::sscanf(buf, " Quality=%s Signal level=%d dBm ESSID:%s\n", quality, &level, essid)) != 3) {
			return vinfo;
		}
		vinfo.push_back(WifiInfo(quality, level, essid));
	}
	std::fclose(fp);

	return vinfo;
}

CameraInfo* HalUtil::openCamera() {
	return new CameraInfo();
}

int HalUtil::closeCamera(CameraInfo *info) {
	delete info;
	return 0;
}

LteInfo* HalUtil::openLte() {
	return new LteInfo();
}

int HalUtil::closeLte(LteInfo *info) {
	delete info;
	return 0;
}
