#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

using namespace std;

int main()
{
    vector<int> xs = { 3,5,1,4,2 };

    sort(begin(xs), end(xs));
    for (auto x : xs) cout << x << " ";
    cout << endl;

    sort(rbegin(xs), rend(xs));
    for (auto x : xs) cout << x << " ";
    cout << endl;
    return 0;
}
