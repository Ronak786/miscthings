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
	char name[50],version[50],arch[20],sigtype[20],packager[50],summary[50],desc[256];
	unsigned long insdate, builddate, size;
	while(std::getline(ifs, line)) {
		if (sscanf(line.c_str(), "name:%s ,version:%s ,arch:%s ,insdate:%lu ,builddate:%lu ,"
					"size:%lu ,sigtype:%s ,packager:%s ,summary:%s ,desc:%s", 
					name, version ,arch, &insdate, &builddate, &size, sigtype, 
					packager, summary, desc) != 10) {
			printf("scanf fail for %s, continue\n", line.c_str());
			continue;
		}
		obj[name]["name"] = name;
		obj[name]["version"] = version;
		obj[name]["arch"] = arch;
		obj[name]["insdate"] = insdate;
		obj[name]["builddate"] = builddate;
		obj[name]["size"] = size;
		obj[name]["sigtype"] = sigtype;
		obj[name]["packager"] = packager;
		obj[name]["summary"] = summary;
		obj[name]["desc"] = desc;
	}
	// check is null, necessary??
	if (!obj.is_null()) {
		ofs << obj;
	} else {
		ofs.close();
	}
	return 0;
}

