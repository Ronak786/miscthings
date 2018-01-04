// 只要左括号比右括号多这个时候一定能放右括号
class Solution {
public:
    vector<string> generateParenthesis(int n) {
        /* for every member string
         * push ( string ) and () string
         */
        vector<string> vs = generateParenthesis(n-1);
        vector<string> vres;
        for (auto str : vs) {
            vres.push_back({str + "()"});
            vres.push_back({"()" + str});
            vres.push_back({"(" + str + ")"});
        }
        return vres;
    }

};
