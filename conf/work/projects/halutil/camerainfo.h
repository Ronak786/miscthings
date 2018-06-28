/*************************************************************************
	> File Name: camerainfo.h
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Thu 28 Jun 2018 02:44:22 PM CST
 ************************************************************************/

#ifndef CAMERAINFO_H
#define CAMERAINFO_H

#include <opencv2/opencv.hpp>

class CameraInfo {

private:
	// should have private init/open/uninit/close function here when called from halutil::open/close
public:
	CameraInfo();
	~CameraInfo();
	int getImage(cv::Mat& mat); //get picture may fail, so use return value here
};
#endif
