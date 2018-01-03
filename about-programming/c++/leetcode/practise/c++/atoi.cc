#include <iostream>
#include <string>
#include <cctype>

using namespace std;

// 所有需要考虑的情况
// 所有负数溢出全部负最大，所有正数溢出全部正最大，注意64位的边界情况
// 去掉开头空格，注意符号获取，注意获取直到整数部分结束 
class Solution {
    public:
        // we only process 32bit signed integers
        int myAtoi(string str) {
                
            int minus = 1;
            char op;
            string digital = getDigit(str, op);
            if (digital == "") {
                return 0;
            }
            if (op == '-') {
                minus = -1;
            }
            return minus * process(digital, op);
        }

        // ignore rest non digital numbers
        string getDigit(string &str, char &op) {
            auto head = begin(str); // '-' symbol
            auto tail = end(str);
            while (isspace(*head)) 
                ++head;
            if (*head == '-' || *head == '+') {
                op = *head;
                ++head;
            } else {
                op = '+';
            }
            auto cur = head;
            while (cur != tail) { //traverse the string 
                if (!isdigit(*cur))
                    break;
                cur++;
            }
            
            return {head, cur};
        }

        int process(string &digital, char op) {
            long result = 0L;

            if (digital.size() >= 11) {//overflow
                if (op == '+')
                    return INT32_MAX;
                else 
                    return INT32_MIN;
            }
            for (auto i : digital) {
                result = result * 10 + i - 48;
            }

            if (result & 0xffffffff80000000) {//overflow
                if (op == '+')
                    return INT32_MAX;
                else 
                    return INT32_MIN;
            }
            return result;
        }

};

int main(int ac, char *av[]) {
    
    Solution so;
    string str{av[1]};
    cout << so.myAtoi(str) << endl;
    return 0;
}
