#include <iostream>

using namespace std;

class A {
    private:
        int a  =20;
    public:
        void hello() {
            cout << " in A " << endl;
            a = 50;
            cout << " a is " << a << endl;
        }
};

class B : public A {
    public:
        void hh() {
            cout << " in B " << endl;
        }
};
int main() {
    B b;
    b.hello();
    return 0;
}
