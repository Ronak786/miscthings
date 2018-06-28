/*************************************************************************
	> File Name: halutil.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 10:38:10 AM CST
 ************************************************************************/


#include "halutil.h"
#include <fstream>
#include <cstdio>

WifiInfo::WifiInfo(std::string quality, int level, std::string essid):
		_quality(quality), _level(level), _essid(essid) {
	_essid.erase(0, 1);
	_essid.erase(_essid.size()-1, 1); // trim double-quotes
	_conffile = "/etc/wpa_supplicant/wpa_supplicant.conf";
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

// currently we have no error check
int	WifiInfo::activate(std::string pass) {
	std::ofstream ofs(_conffile, std::ios::out | std::ios::trunc);	
	ofs << "network {" << std::endl;
	ofs << "	ssid=\"" << _essid << "\"" << std::endl;
	ofs << "	psk=\"" << pass << "\"" << std::endl;
	ofs << "}" << std::endl;
	ofs.close();
	system("dhcpcd > /dev/null 2>&1 "); // start dhcpcd
	return 0;
}

int	WifiInfo::deactivate() {
	std::remove(_conffile.c_str());
	system("killall dhcpcd");
	return 0;
}

// return string in xxx.xx/xx
std::string	WifiInfo::getIp() {
	return std::string("192.168.0.123/24");
}

