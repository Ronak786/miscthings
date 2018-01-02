#include <string>
#include <iostream>

using namespace std;
/*
 * be careful 回文有两种，一种是自己中心，一种是左右偶数对称，对每个都要这么检验，目前是这样的
 */
class Solution {
public:
    string longestPalindrome(string s) {
	    int siz = s.size();
        int maxlength = 0;
        string maxstring;
        // we can shorten this loop when the maxlength/2 is bigger than the remaining loop times
        for (int i = 0; i < siz; ++i) {
            string tmpstring = getPalindrome(s, i);
            if (tmpstring.length() > maxlength) {
                maxlength = tmpstring.length();
                maxstring = tmpstring;
            }
        }
        return maxstring;
    }

    string getPalindrome(string& src, int idx) {
        int left1=0, right1=0, left2=0, right2=0;
        int length1 = 0, length2 = 0;
        string tmpstr1, tmpstr2;

        if (idx != 0 && (src[idx] == src[idx-1])) { //double
            left1 = idx-2;
            right1 = idx+1;
            length1 = 2;
            while (left1 >= 0 && right1 < src.length()) {
                if (src[left1] == src[right1]) {
                    length1 += 2;
                    left1--;
                    right1++;
                } else {
                    break;
                }
            }
        } 
        //single  must have
        left2 = idx-1;
        right2 = idx+1;
        length2 = 1;

        while (left2 >= 0 && right2 < src.length()) {
            if (src[left2] == src[right2]) {
                length2 += 2;
                left2--;
                right2++;
            } else {
                break;
            }
        }
        if (length1 > length2) { //double
            return {begin(src) + left1+1, begin(src) + right1};
        } else {
            return {begin(src) + left2+1, begin(src) + right2};
        }
    }
};

int main() {
    Solution so;
    string ss{"ccc"};
    cout << so.longestPalindrome(ss) << endl;
    return 0;
}
