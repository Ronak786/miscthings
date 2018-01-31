#include "local.h"

int main() {
    int head = 0;
    int tail = 100;
    int mid = 50;
    string reply;
    cout << mid << " ?" << endl;
    while (true) {
        cin >> reply;
        if (reply == "h") {
            tail = mid-1;
        } else if (reply == "l") {
            head = mid+1;
        } else {
            cout << "success" << endl;
            break;
        }
        mid = head + (tail - head)/2;
        cout << mid << " ?" << endl;
    }
    return 0;
}
