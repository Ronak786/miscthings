#include <iostream>
#include <string>
#include <vector>

using namespace std;
/* 另一种做法是每次比较每个string的同一个位置的字符，全部成功才进位，否则直接返回当前匹配完成的prefix
class Solution {
public:
    string longestCommonPrefix(vector<string>& strs) {
        if (strs.size() == 0) return {""};
        if (strs.size() == 1)
            return strs[0];
        return calPrefix(strs[0], next(begin(strs)), end(strs));
    }

    string calPrefix(string first, vector<string>::iterator head, vector<string>::iterator tail) {
        if (tail - head == 0) return first;
        string second = *head;
        int iter = 0;
        while (first[iter] == second[iter]) {
            iter++;
            if (iter == first.size() || iter == second.size())
                break;
        }
        if (tail - head == 1) {
            return {begin(first), begin(first) + iter};
        } else {
            return calPrefix({begin(first), begin(first) + iter}, next(head), tail);
        }
    }
};

int main() {
    vector<string> vs{"bianminling", "bianming", "abia", "bian"};
    Solution so;
    cout << so.longestCommonPrefix(vs) << endl;
    return 0;
}
