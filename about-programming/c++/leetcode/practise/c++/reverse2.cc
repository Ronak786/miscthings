#include <iostream>
#include <numeric>
#include <cstdint>
#include <cstdio>

using namespace std;

//to check a 32bit signed overflow, we can use 64bit and check if the 33-63bit is set
class Solution {
    public:
    int reverse(int x) {
        int flag = 1;
        long orig = x;
        if (x == INT32_MIN) return 0;

        if (x < 0) { // todo : int.min = -xxxx have not corresponding positive number , but we have to deal it !!
            flag = -1;
            orig = -x;
        }

        long result = 0;
        do {
            result = result*10 + orig%10;
            orig /= 10;
            cout << "last " << result << endl;
        } while (orig != 0);
        if (result & 0xffffffff80000000)
            return 0;
        return flag * result;
    }
};

int main(int ac, char *av[]) {
    int num = atoi(av[1]);
    Solution so;
    cout << "result " << so.reverse(num) << endl;
    return 0;
}
