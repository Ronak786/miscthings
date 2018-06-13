/*************************************************************************
	> File Name: a.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Wed 13 Jun 2018 05:18:17 PM CST
 ************************************************************************/

#include "json.h"

#include <fstream>
#include <iostream>

using json = nlohmann::json;
using namespace std;

int main(int ac, char *av[]) {
	ifstream ifs(av[1]);
	json jj;
	ifs >> jj;
	for (json::iterator it = jj.begin(); it != jj.end(); ++it) {
		cout << "iterate: " << it.key() << endl;
		if (it.key() == "one") {
			cout << " we have one" << endl;
		}
	}
	return 0;
}
