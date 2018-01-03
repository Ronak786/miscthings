#include <iostream>
#include <string>

using namespace std;
class Solution {
    public:
        // without extra space
        // if negative => false
        // to_string
        // from start and end check until same or cross
        bool isPalindrome(int x) {
            if (x < 0) return false;
            string str{to_string(x)};
            for (auto b = begin(str), e = prev(end(str)); b < e ; ++b, --e) {
                if (*b != *e) 
                    return false;
            }
            return true;
        }
};

int main(int ac, char *av[]) {
    int k = atoi(av[1]);
    Solution so;
    cout << so.isPalindrome(k) << endl;
    return 0;
}

