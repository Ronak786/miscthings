#include <iostream>
#include <vector>
#include <set>
#include <queue>

/* 每次往前挪到当前可以到达的最前方，同时更新当前按照步数下次可以到达的最大值，知道最前方已经到达最大值了位置
 int jump(int A[], int n) {
	 if(n<2)return 0;
	 int level=0,currentMax=0,i=0,nextMax=0;

	 while(currentMax-i+1>0){		//nodes count of current level>0
		 level++;
		 for(;i<=currentMax;i++){	//traverse current level , and update the max reach of next level
			nextMax=max(nextMax,A[i]+i);
			if(nextMax>=n-1)return level;   // if last element is in level+1,  then the min jump=level 
		 }
		 currentMax=nextMax;
	 }
	 return 0;
 }
*/
using namespace std;

class Solution {
public:
    int jump(const vector<int>& nums) {
       set<int> si;
       queue<int> qi, tmpi;
       qi.push(0);
       si.insert(0);
       int loop = 0;
       while (si.find(nums.size()-1) == end(si)) {
           loop++; // must have another jump
           while (!qi.empty()) {
               int idx = qi.front();
               qi.pop();
               for (int i = 1; idx < nums.size() && i <= nums[idx]; ++i) {
                   if (si.find(idx+i) == end(si)) {
                       tmpi.push(idx + i); // add to queue for next pop and add set
                   }
                   si.insert(idx + i); // add to set for find
                   cout << "in loop " << loop << " add " << idx + i << endl;
               }
           }
           qi.swap(tmpi);
       }
       return loop;
    }
};

int main() {
    Solution so;
//    cout << so.jump({8,2,4,4,4,9,5,2,5,8,8,0,8,6,9,1,1,6,3,5,1,2,6,6,0,4,8,6,0,3,2,8,7,6,5,1,7,0,3,4,8,3,5,9,0,4,0,1,0,5,9,2,0,7,0,2,1,0,8,2,5,1,2,3,9,7,4,7,0,0,1,8,5,6,7,5,1,9,9,3,5,0,7,5}) << endl;
    cout << so.jump({2,3,1,1,4}) << endl;
    return 0;
}
