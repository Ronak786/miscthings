class Solution {
    public:
        double FindMedianSortedArrays(vector<int>& nums1, vector<int>& nums2) {
            return FindMedian(begin(nums1), end(nums1), begin(nums2), end(nums2), (nums1.size() + nums2.size()) %2);
        }

        double FindInOne(vector::iterator b1, vector::iterator e1) {
            return b1 + (e1 - b1)/2;
        }

        double FindMedian(vector::iterator b1, vector::iterator e1, vector::iterator b2, vecor::iterator e2, bool odd) {
            auto mid1 = b1 + (e1 - b1)/2;
            auto mid2 = b2 + (e2 - b2)/2;
            if (*mid1 <= *mid2) {
                if (mid1 != e1 && b2 != mid2) {
                    return FindMedian(mid1, e1, b2, mid2, odd);
                } else if (mid1 == e1) {
                    return FindInOne(b2, mid2);
                } else {
                    return FindInOne(mid1, e1);
                }
            } else {
                if (b1 != mid1 && mid2 != e2) {
                    return FindMedian(b1, mid1, mid2, e2, odd);
                } else if (b1 == mid1) {
                    return FindInOne(mid2, e2);
                } else {
                    return FindInOne(b1, mid1);
                }
            }
        }
};

int main() {
    Solution so;

    vector<int> v1{1,2};
    vector<int> v2{3,4};
    cout << FindMedianSortedArrays(v1, v2) << endl;
    return 0;
}
