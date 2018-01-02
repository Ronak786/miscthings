#include <iostream>
#include <vector>
#include <algorithm>

/*
 * 排除法
 * 具体来说就是把题目转化为寻找第k大的数，
 * 然后每次通过比对中间数去掉最小的那一小半
 */
using namespace std;

class Solution {
    public:
        double FindMedianSortedArrays(vector<int>& nums1, vector<int>&nums2) {
            int total = nums1.size() + nums2.size();
            if (total % 2) {
                return FindKth(begin(nums1), end(nums1), begin(nums2), end(nums2), total/2+1);
            } else {
                return (FindKth(begin(nums1), end(nums1), begin(nums2), end(nums2), total/2) + 
                        FindKth(begin(nums1), end(nums1), begin(nums2), end(nums2), total/2+1)) /2;
            }
        //    return FindKth(begin(nums1), end(nums1), begin(nums2), end(nums2), total/2+1, total%2);
        }

        double FindKth(vector<int>::iterator b1, vector<int>::iterator e1, vector<int>::iterator b2, vector<int>::iterator e2, int k) {
            if (e1 - b1 > e2 - b2) {
                return FindKth(b2, e2, b1, e1, k);
            }
            if (b1 == e1) {
                return *(b2 + k -1);
            }
            if (k == 1) {
                return min(*b1, *b2);
            }
            int pa = min((int)(e1 - b1), k/2);
            int pb = k - pa;
            if (*(b1 + pa-1) < *(b2 + pb -1)) {
                return FindKth(b1+pa, e1, b2, e2, k - pa);
            } else if (*(b1 + pa-1) > *(b2 + pb-1)) {
                return FindKth(b1, e1, b2+pb, e2, k-pb);
            } else 
                return *(b1 + pa-1);
        }
/*
        double FindKth(vector<int>::iterator b1, vector<int>::iterator e1, vector<int>::iterator b2, vector<int>::iterator e2, int k, bool odd) {
            if (e1 - b1 > e2 - b2) {
                return FindKth(b2, e2, b1, e1, k, odd);
            }
            if (b1 == e1) {
                return odd ? *(b2 + k -1) : (*(b2 + k -1) + *(b2 + k)) /2;
            }
            if (k == 1) {
                if (!odd) {
                    if (e1 - b1 == 1) {
                        return (*b1 + *b2) / 2.0;
                    } else {
                        return (min(*b1, *b2) + min(max(*b1, *b2), *(b1+1))) / 2.0;
                    }
                } else {
                    return min(*b1, *b2);
                }
            }
            int pa = min((int)(e1 - b1), k/2);
            int pb = k - pa;
            if (*(b1 + pa-1) < *(b2 + pb -1)) {
                return FindKth(b1+pa, e1, b2, e2, k - pa, odd);
            } else if (*(b1 + pa-1) > *(b2 + pb-1)) {
                return FindKth(b1, e1, b2+pb, e2, k-pb, odd);
            } else  {
                if (!odd) {
                    if (e1 - b1 - pa + 1 == 1) {
                        return (*(b1 + pa-1) + *(b2 + pb -1)) / 2.0;
                    } else {
                        return (*(b1 + pa -1) + min(max(*(b1+pa-1), *(b2+pb-1)), *(b1+pa))) / 2.0;
                    }
                } else {
                    return *(b1 + pa-1);
                }
            }
        }
*/
};

int main() {
    Solution so;

    vector<int> v1{1,3};
    vector<int> v2{2, 4};
    cout << so.FindMedianSortedArrays(v1, v2) << endl;
    return 0;
}
