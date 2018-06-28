/*************************************************************************
	> File Name: halutil.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 11:04:19 AM CST
 ************************************************************************/

#include <cstdio>

#include "halutil.h"

int HalUtil::getAvailWifiList(std::vector<WifiInfo> & vinfo) {
	std::string cmdstr = "iwlist wlp3s0 scanning | egrep '(ESSID|Signal level=)' | sed  'N;s/\\n/ /'";
	std::FILE* fp = popen(cmdstr.c_str(), "r");
	if (!fp) {
		printf("read pipe error\n");
		return -1;
	}

	// used to store result line
	char buf[256];
	int count;

	char quality[20], essid[128];
	int level;
	while (std::fgets(buf, sizeof(buf)-1, fp) != NULL) {
		if ((count = std::sscanf(buf, " Quality=%s Signal level=%d dBm ESSID:%s\n", quality, &level, essid)) != 3) {
			return -2;
		}
		vinfo.push_back(WifiInfo(quality, level, essid));
	}
	std::fclose(fp);

	return 0;
}
