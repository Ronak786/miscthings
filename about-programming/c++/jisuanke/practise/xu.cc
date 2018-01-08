#include <iostream>

using namespace std;

class A;
using func = void(*)(void*);
//typedef void(*func)(void);
//typedef int(A::*func)();

class A {
    public:
        virtual void f() {cout << "f" << endl;}
        virtual void g() {cout << "g" << endl;}
        virtual void k(int q) { cout << "k " << q << endl;}
};

int main() {
    A a;
    a.f();
    cout << (int*)&a << endl; //虚表地址
    cout << (int*)*((int*)&a) << endl; //地一个函数地址
    func pfunc =  (func)(int*)*((int*)&a);
    pfunc(&a);
    return 0;
}
