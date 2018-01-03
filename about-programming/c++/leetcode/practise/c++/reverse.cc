#include <iostream>
#include <numeric>
#include <cstdint>

using namespace std;

//to check a 32bit signed overflow, we can use 64bit and check if the 33-63bit is set
// but in this ,I use the add everytime instead of mul to check, if once negative, then overflow
// if just use mul, it will handle the overflow and you will finally get a positive or negative arbitary number
class Solution {
    public:
    int reverse(int x) {
        int flag = 1;
        int orig = x;
        if (x == INT32_MIN) return 0;

        if (x < 0) { // todo : int.min = -xxxx have not corresponding positive number , but we have to deal it !!
            flag = -1;
            orig = -x;
        }

        int result = 0;
        do {
            for (int i = 0, tmp = result; i < 9; ++i) {
                result += tmp;
                if (result < 0)
                    return 0;
            }
            result += orig%10;
            if (result < 0)
                return 0;
            orig /= 10;
        } while (orig != 0);
        return flag * result;
    }
};

int main(int ac, char *av[]) {
    int num = atoi(av[1]);
    Solution so;
    cout << so.reverse(num) << endl;
    return 0;
}
