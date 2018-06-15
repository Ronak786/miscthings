/*************************************************************************
	> File Name: vowel.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Fri 15 Jun 2018 01:52:43 PM CST
 ************************************************************************/

#include <iostream>
#include <string>
#include <set>
#include <vector>

std::string disemvowel(std::string str)
{
	std::set<char> ss({'a','e','i','o','u','A','E','I','O','U'});
	std::vector<char> vc;
	for(auto chr: str) {
		if (ss.find(chr) == ss.end()) {
			vc.push_back(chr);
		}
	}
	std::string res;
	res.reserve(str.size());
	for (auto i: vc) {
		res += i;
	}
	return res;
    // return
}

int main(int ac, char* av[]){ 
	std::string res = disemvowel("aThis website is for losers LOL!");
	std::cout << res << std::endl;
	return 0;
}
