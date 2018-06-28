/*************************************************************************
	> File Name: camerainfo.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 02:44:22 PM CST
 ************************************************************************/

#ifndef LTEINFO_H
#define LTEINFO_H

#include <string>

class LteInfo {

private:
	// should have private init/open/uninit/close function here when called from halutil::open/close

public:
	LteInfo();
	~LteInfo();
	std::string getIp(); // return ip addr
	int getSignalStrength(); // return signal strength
};
#endif
