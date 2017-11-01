#include <iostream>

class A {
    protected:
        int a;
    public:
        A(int tmp) : a(tmp) {}
};

class B : public A {
    public:
        B(int tmp) : A(tmp) {}
        void test() {
            std::cout << this->a << std::endl;
        }
};

int main() {
    B b(20);
    b.test();
    return 0;
}
