#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <list>

using namespace std;

template<typename T>
void BubbleSort(T first, T last)
{
    if (first == last || next(first) == last) return;

    auto limitlast = prev(last);
    for (auto i = first; i != prev(last); ++i) {
        for (auto k = first; k != limitlast; ++k) {
            if (*k > *next(k))
                swap(*k, *next(k));
        }
        limitlast = prev(limitlast);
    }
}

int main()
{
    {
        vector<int> xs = { 3,5,1,4,2 };
        BubbleSort(begin(xs), end(xs));

        for (auto x : xs)cout << x << " ";
        cout << endl;
    }
    {
        list<int> xs = { 3,5,1,4,2 };
        BubbleSort(rbegin(xs), rend(xs));

        for (auto x : xs)cout << x << " ";
        cout << endl;
    }
    {
        int xs[] = { 7,3,5,1,4,2,0 };
        BubbleSort(&xs[1], &xs[6]);

        for (auto x : xs)cout << x << " ";
        cout << endl;
    }
    return 0;
}
