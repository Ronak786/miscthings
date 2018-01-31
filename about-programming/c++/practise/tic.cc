#include "local.h"

class Pad {
    private:
        vector<int> vi;
    public:
        Pad() {
            for (int i = 0; i < 9; ++i) {
                vi.push_back(0);
            }
        }
        void set(int i, int j, int target) {
            vi[(i-1)*3 + j-1] = target;
        }

        int get(int i, int j) {
            return vi[(i-1)*3 + j-1];
        }

        void printPad() {
            for (int i = 1; i <= 3; ++i) {
                for (int j = 1; j <= 3; ++j) {
                    cout << " | " << vi[(i-1)*3+j-1];
                }
                cout << " |" << endl;
            }
        }

        bool check(int target) {
            if ((get(1,1) == target && get(1,2) == target && get(1,3) == target) ||
                    (get(2,1) == target && get(2,2) == target && get(2,3) == target) ||
                    (get(3,1) == target && get(3,2) == target && get(3,3) == target) ||
                    (get(1,1) == target && get(2,1) == target && get(3,1) == target) ||
                    (get(1,2) == target && get(2,2) == target && get(3,2) == target) ||
                    (get(1,3) == target && get(2,3) == target && get(3,3) == target) ||
                    (get(1,1) == target && get(2,2) == target && get(3,3) == target) ||
                    (get(1,3) == target && get(2,2) == target && get(3,1) == target) )
                return true;
            return false;
        }
};

int main() {
    Pad pad;
    pad.printPad();
    int x;
    int y;
    random_device rd;
    default_random_engine e(rd());
    uniform_int_distribution<> ue(1,3);
    while (cin >> x && cin >> y) {
        pad.set(x, y, 1);
        pad.printPad();
        if (pad.check(1)) {
            cout << "you win" << endl;
            break;
        }
        sleep(1);
        cout << "computer: " << endl;
        int cx = ue(e);
        int cy = ue(e);
        while (pad.get(cx, cy) != 0) {
            cx = ue(e);
            cy = ue(e);
        }
        pad.set(cx, cy, 2);
        pad.printPad();
        if (pad.check(2)) {
            cout << "computer win" << endl;
            break;
        }
    }
    return 0;
}
