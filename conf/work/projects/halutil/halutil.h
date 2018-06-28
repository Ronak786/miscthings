/*************************************************************************
	> File Name: halutil.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 11:03:01 AM CST
 ************************************************************************/


#ifndef HALUTIL_H
#define HALUTIL_H
#include "wifiinfo.h"
#include <vector>

class HalUtil {
public:
	static int getAvailWifiList(std::vector<WifiInfo>& list);

};

#endif
