#include <iostream>
#include <list>

template<typename T>
void ploop(T t) {
    for (auto i : t) {
        std::cout << i << std::endl;
    }
}

class A {
    private:
        int pp = 0;
    protected:
        int a;
    public:
        A(int tmp) : a(tmp) {}
        void set(int qq) { pp = qq; }
};

class B : public A {
    public:
        B(int tmp) : A(tmp) {}
        void test() {
            std::cout << this->a << std::endl;
        }
};

int main() {
    B b(30);
    b.set(20);
    return 0;
}
