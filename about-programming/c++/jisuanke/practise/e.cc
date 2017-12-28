#include <iostream>

using namespace std;

class A {
    private:
        int a;
    public:
        A(int e): a{e} {}
        void print() {
            cout << a << endl;
        }
        class B {
            private:
                int b;
            public:
                void setA(A& f) {
                   f.a = 20;
                }
        };
};

int main() {
    A a{10};
    a.print();
    A::B b;
    b.setA(a);
    a.print();
    return 0;
}
    
