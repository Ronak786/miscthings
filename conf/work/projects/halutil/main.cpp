/*************************************************************************
	> File Name: main.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 10:44:53 AM CST
 ************************************************************************/


#include "halutil.h"
#include <cstdio>
#include <vector>

int main() {
	std::string cmdstr = "iwlist wlp3s0 scanning | egrep '(ESSID|Signal level=)' | sed  'N;s/\\n/ /'";
	std::FILE* fp = popen(cmdstr.c_str(), "r");
	if (!fp) {
		printf("read pipe error\n");
		return -1;
	}

	std::vector<WifiInfo> vinfo;
	char buf[256];
	int count;

	char quality[20], essid[128];
	int level;
	while (std::fgets(buf, sizeof(buf)-1, fp) != NULL) {
		if ((count = std::sscanf(buf, " Quality=%s Signal level=%d dBm ESSID:%s\n", quality, &level, essid)) != 3) {
			printf("scanning ;%s; error, count: %d\n", buf, count);
			return -1;
		}
		printf("scanf get :%s: :%d: :%s:\n", quality, level, essid);
		vinfo.push_back(WifiInfo(quality, level, essid));
	}
	std::fclose(fp);

	for (auto wifiinfo: vinfo) {
		printf("vinfo: ;%s; ;%d; ;%s;\n", wifiinfo.getQuality().c_str(), wifiinfo.getLevel(), wifiinfo.getEssid().c_str());
	}
	return 0;
}
