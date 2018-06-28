/*************************************************************************
	> File Name: halutil.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 10:38:10 AM CST
 ************************************************************************/


#include "halutil.h"

WifiInfo::WifiInfo(std::string quality, int level, std::string essid):
		_quality(quality), _level(level), _essid(essid) {
	_essid.erase(0, 1);
	_essid.erase(_essid.size()-1, 1); // trim double-quotes
}

WifiInfo::~WifiInfo() {}

std::string WifiInfo::getQuality() {
	return _quality;
}

int	WifiInfo::getLevel() {
	return _level;
}

std::string WifiInfo::getEssid() {
	return _essid;
}
