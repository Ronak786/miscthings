/*************************************************************************
	> File Name: sortarray.cpp
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Fri 15 Jun 2018 11:24:49 AM CST
 ************************************************************************/

#include<iostream>
#include <vector>
#include <map> //ordered by key
#include <algorithm>
 
class Kata
{
public:
    std::vector<int> sortArray(std::vector<int> array)
    {
		std::map<int, int> oddmap;
		std::vector<int>  sortvec;
        for(int i = 0; i < array.size(); ++i) {
			if (array[i] % 2 != 0) {
				oddmap[i] = array[i];
				sortvec.push_back(i);
//				std::cout << "pushdback: " << i << std::endl;
			}
	    }
		std::sort(sortvec.begin(), sortvec.end(), [&array](int a, int b) {
			return array[a] < array[b];
		});
		
		int i = 0;
		for(auto tmppair: oddmap) {
			array[tmppair.first] = oddmap[sortvec[i]];
//			std::cout << "put into index " << tmppair.first << " value " << oddmap[sortvec[i]] << std::endl;
			++i;
		}
        return array;
    }
};


int main(int ac, char*av[]) {
//	std::vector<int> input{5, 3, 2, 8, 1, 4 };
	std::vector<int> input{5, 3, 1, 8, 0};
	Kata tmp;
	std::vector<int> output = tmp.sortArray(input);
	for (auto i : output) {
		std::cout << i << " ";
	}
	std::cout << std::endl;
	return 0;
}
