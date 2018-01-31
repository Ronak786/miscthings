#include "local.h"

int main() {
    int input;
    int count = 0;

    while (true) {
        cout << "input " << count <<  endl;
        cin >> input;
        if (input == count) {
            cout << "stop" << endl;
            break;
        }
        count++;
    }
    return 0;
}
