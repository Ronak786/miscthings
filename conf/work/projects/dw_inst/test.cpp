/*************************************************************************
	> File Name: test.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Mon 11 Jun 2018 01:16:53 PM CST
 ************************************************************************/

#include <iostream>
#include <fstream>

using namespace std;

int main() {
	ofstream ofs("hehe");
	if (!ofs) {
		printf("can not get file hehe to write\n");
	} else { 
		printf("success get file\n");
	}
	return 0;
}
