/*************************************************************************
	> File Name: createjsonfromtext.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Mon 11 Jun 2018 09:23:28 AM CST
 ************************************************************************/

#include<iostream>
#include <fstream>
#include <cstdio>
#include "json.h"
using namespace std;
using json = nlohmann::json;

int main(int ac, char *av[]) {
	if (ac != 3) {
		printf("need input && output file name para\n");
		return -1;
	}

	ifstream ifs(av[1]);
	ofstream ofs(av[2]);
	if (!ifs || !ofs) {
		printf("can not open two files\n");
		return -1;
	}

	string line;
	json obj({});
	char name[50],version[50],size[50];
	while(std::getline(ifs, line)) {
		if (sscanf(line.c_str(), "name:%s version:%s size:%s", name, version ,size) != 3) {
			printf("scanf fail for %s, continue\n", line.c_str());
			continue;
		}
		obj[name]["name"] = name;
		obj[name]["version"] = version;
		obj[name]["size"] = size;
	}
	if (!obj.is_null()) {
		ofs << obj;
	} else {
		ofs.close();
	}
	return 0;
}

