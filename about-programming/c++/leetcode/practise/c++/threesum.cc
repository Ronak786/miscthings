// 这个方法跳过了所有的重复可能，包括twosum时候的重复，以及
// 当前作为第三个数可能的重复
vector<vector<int>> threeSum(vector<int> & num) {
    sort(num.begin(), num.end());
    for (int i = 0; i < num.size(); ++i) {
        int pick = num[i];
        vector<vector<int>> vi;

        int left = i+1;
        int right = num.size()-1;

        if (i > 0 && num[i] == num[i-1]) continue;
        while (left < right) {

            int sum = pick + num[left] + num[right];
            if (sum < 0) {
                left++;
            } else if (sum > 0) {
                right--;
            } else {
               vi.push_back({pick, num[left], num[right]});
               while (left+1 < right && num[left] == num[left+1]) left++;
               while (right-1 > left && num[right] == num[right-1]) right--; 
               right--;
               left++;
            }
        }
    }
    return vi;
}
