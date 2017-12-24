/*
map<string, vector<int>> mm;
while (getinput) {
    mm.get(name).second.push_back(grade);
}
for every pair in map:
    sort vevtor and calculate avg
    push name and avg into priority queue
get the top three from pq;
    pq member is pair<string, double>
*/
#include <iostream>
#include <algorithm>
#include <map>
#include <vector>
#include <queue>
#include <numeric>

using namespace std;

int main() {
    int count;
    cin >> count;
    map<string, vector<double>> gradeMap;
    auto mycmp = [](auto a, auto b) {return a.second > b.second;};
    priority_queue<pair<string, double>, decltype(mycmp)> pq(mycmp);
    for (int i = 0; i < count; ++i) {
        string name;
        double grade;
        cin >> name >> grade;
        auto search = gradeMap.find(name);
        if (search == end(gradeMap)) {
            gradeMap.insert(make_pair(name, vector<double>{grade}));
        } else {
            search->second.push_back(grade);
        }
    }

    for (auto key : gradeMap) {
        sort(rbegin(key.second), rend(key.second));
        auto avg = accumulate(begin(key.second), begin(key.second) + min(5, key.second.size()), 0.0, 
                [](double a, double b) { return a + b;}) / min(5, key.second.size());
        pq.push(make_pair(key.first, avg));
    }

    for (int i = 0; i < 3; ++i){
        auto pick = pq.top();
        cout << pick.first << " " << pick.second << endl;
        pq.pop();
    }
    return 0;
}

