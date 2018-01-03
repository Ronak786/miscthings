#include <iostream>
#include <string>

using namespace std;
// every time we can deduce two numbers 
// from every linestart, one for idx and other for idx + (r-1-currow)*2
// then idx += (r-1)*2 to the next column
class Solution {
    public:
        string convert(string s, int numRows) {
            int r = numRows;
            int siz = s.size();
            string result;
            result.reserve(s.size());
            if (siz == 0)
                return {};
            if (numRows == 1)
                return s;
            for (int curRow = 0; curRow < r; ++curRow) {
                int idx = curRow;

                //process every line
                while (idx < siz) {
//                    cout << "append " <<  idx << " " << s[idx] << endl;
                    result.append(1, s[idx]);
                    if (curRow != 0 && curRow != r-1 && (idx + (r-1-curRow)*2 < siz)) {
                        result.append(1, s[idx + (r-1-curRow)*2]);
 //                       cout << "append " << idx + (r-1-curRow)*2 << " " << s[idx + (r-1-curRow)*2] << endl;
                    }
                    idx += (r-1)*2;
  //                  cout << "now idx is " << idx << endl;
                }
            }
            return result;
        }
};


int main(void) {
    string s = "";
    Solution so;
    cout << so.convert(s, 1) << endl;
    return 0;
}
