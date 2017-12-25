#include <iostream>
#include <string>

using namespace std;

struct point {
    int x{2};
    int y{3};
//    point (int z, int w): x(z), y(w){}
//    point (): x(20), y(20) {}
};

int main()
{
    point p;
    cout << p.x << " " << p.y << endl;
    return 0;
}
