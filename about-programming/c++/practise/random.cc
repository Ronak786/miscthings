#include "local.h"

int main() {
    random_device rd;
    default_random_engine e(rd());
    uniform_int_distribution<int> uniform_dist(1,100);
    while (true) {
        cout << uniform_dist(e) << endl;
        sleep(1);
    }
    return 0;
}
