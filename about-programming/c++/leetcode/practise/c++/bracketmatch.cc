#include <stack>
#include <iostream>
#include <string>

using namespace std;

class Solution {
    public :
        bool isValid(string s) {
            string left = "({[";
            string right = ")}]";
            stack<char> sc;
            for (auto i : s) {
                if (left.find(i) != string::npos) {
                    sc.push(i);
                } else if (right.find(i) != string::npos) { // have a match maybe
                    if (sc.empty()) return false;
                    auto pos = right.find(i);
                    switch(pos) {
                        case 0: if (sc.top() != '(') return false; break;
                        case 1: if (sc.top() != '{') return false; break;
                        case 2: if (sc.top() != '[') return false; break;
                        default: cout << "error switch" << endl; break;
                    }
                    sc.pop();
                } else {
                    continue;
                }
            }
            return sc.empty() ? true:false;
        }
};

int main() {
    string s = "]";
    Solution so;
    cout << so.isValid(s) << endl;
    return 0;
}
