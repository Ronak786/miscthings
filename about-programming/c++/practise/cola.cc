#include "local.h"

int main() {
    int idx;
    vector<string> vs{"a", "b", "c", "d", "e"};
    cout << "input your favor" << endl;
    cin >> idx;
    switch(idx) {
        case 1: 
        case 2:
        case 3:
        case 4:
        case 5:
            cout << "your choosing is " << vs[idx-1] << endl;
        default:
            break;
    }
    /*
    if (idx >0 && idx < (int)vs.size()) {
        cout << "your choosing is " << vs[idx-1] << endl;
    } else {
        ;
    }
    */
    return 0;
}
