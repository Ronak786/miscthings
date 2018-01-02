#include <iostream>
#include <string>
#include <set>

/*
 * use iterator the traverse, use set to track duplicate
 * if the same, erase from set until the same one, 
 */
using namespace std;

class Solution {
    public:
        int lengthOfLongestSubstring(string s) {
            if (s.empty()) return 0;

            int maxlength = 1;
            auto endIt = end(s);
            auto startIt = begin(s);
            auto tailIt = next(startIt);
            set<string::value_type> sc{*startIt};
            int curlength;
            bool flag = false;
            while (tailIt != endIt) { //we have to count
               if (sc.find(*tailIt) == end(sc)) { // if not in set, insert and add iter
                   sc.insert(*tailIt);
                   tailIt++;
                   flag = true;
               } else { // if in set, delete until the equal one(inclusively), then add the deleted one
                   if (flag) {
                       curlength = tailIt - startIt;
                       maxlength = maxlength > curlength ? maxlength : curlength;
                       flag = false;
                   }
                   while (*startIt != *tailIt) {
                       sc.erase(*startIt++);
                   };
                   startIt++;
                   tailIt++;
               }
            }
            curlength = tailIt - startIt;
            maxlength = maxlength > curlength ? maxlength : curlength;
            return maxlength;
        }
};

int main() {
    string s{"abcabcbb"};
    Solution so;
    cout << so.lengthOfLongestSubstring(s) << endl;
    return 0;
}
