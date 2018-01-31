#include "local.h"

struct people {
    int id;
    int eat;
    bool operator< (people two) {
        return eat < two.eat;
    }
    bool operator> (people two) {
        return eat > two.eat;
    }
};


int main() {
    int eaten;
    int _max, _min;
    _max = _min = 0; 
    vector<people> vp;
    cout << "input 10 cake eaten" << endl;
    for (int  i = 0; i < 10; ++i) {
        cin >> eaten;
        vp.push_back({i, eaten});
        if (vp[i] < vp[_min]) _min = i;
        if (vp[i] > vp[_max]) _max = i;
    }
    cout << "max is person " << vp[_max].id << endl;
    cout << "min is person " << vp[_min].id << endl;
    sort(vp.begin(), vp.end(), [](auto one, auto two) {
            return one.eat < two.eat;
            });
    for (auto i = 0; i < (int)vp.size(); ++i) {
        printf("Person %d: ate %d pancakes\n", vp[i].id, vp[i].eat);
    }
    return 0;
}

