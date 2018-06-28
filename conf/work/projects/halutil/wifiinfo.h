/*************************************************************************
	> File Name: halutil.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 10:35:15 AM CST
 ************************************************************************/


#ifndef WIFIINFO_H
#define WIFIINFO_H
#include <string>
#include <vector>

class WifiInfo {
private:
	std::string _conffile;
	std::string _quality;
	int			_level;
	std::string _essid;
public:
	WifiInfo(std::string quality, int level, std::string essid);
	~WifiInfo();
	std::string getQuality();
	int			getLevel();
	std::string getEssid();
	int			activate(std::string pass);
	int			deactivate();
	std::string	getIp();

};
#endif
