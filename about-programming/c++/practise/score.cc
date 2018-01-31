#include "local.h"

class Programming {
    private:
        int score;
    public:
        Programming(int s): score(s) {
            if (s == 100) {
                cout << "you got a prefect sore" << endl;
            } else if ( s >= 90) {
                cout << "you got an A" << endl;
            } else if (s >= 80) {
                cout << "B" << endl;
            } else if (s >= 70) {
                cout << "C" << endl;
            } else if (s >= 60) {
                cout << "D" << endl;
            } else if (s >= 50) { 
                cout << "F" << endl;
            }
        }

        ~Programming(){}
};

int main() {
    int score;
    cout << "input your score" << endl;
    while (cin >> score) {
        Programming prog(score);
    }
    return 0;
}

