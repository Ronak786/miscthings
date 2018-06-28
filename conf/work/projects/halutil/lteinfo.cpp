/*************************************************************************
	> File Name: camerainfo.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 02:45:46 PM CST
 ************************************************************************/

#include "lteinfo.h"

LteInfo::LteInfo() {
}

LteInfo::~LteInfo() {
}

std::string LteInfo::getIp() {
	return std::string("61.160.81.30/24");
}

int LteInfo::getSignalStrength() {
	return 3;
}
