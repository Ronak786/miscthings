#include <vector>
#include <algorithm>
#include <iostream>
#include <utility>
using namespace std;

/*
 * use a pair<number, position> and sort that
 * compare the first and if match, print the second
 */
class Solution {
    public:
        vector<int> twoSum(vector<int>& nums, int target) {
            vector<pair<int, int>> vp;
            for (auto i = 0; i < nums.size(); ++i) {
                vp.push_back(make_pair(nums[i], i));
            }
            sort(begin(vp), end(vp));

            auto left = begin(vp);
            auto right = end(vp) - 1;
            while (left < right) {
                if (left->first + right->first < target) {
                    left++;
                } else if (left->first + right->first > target) {
                    right--;
                } else {
                    return {min(left->second, right->second), max(left->second, right->second)};
                }
            }
            return {};
        }
};

int main() {
    Solution sl;
    vector<int> vi{3,2,4};
    int target = 6;
    for (auto i : sl.twoSum(vi, target)) {
        cout << i << endl;
    }
    return 0;
}
