#include <iostream>
#include <list>

template<typename T>
void ploop(T t) {
    for (auto i : t) {
        std::cout << i << std::endl;
    }
}

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
    std::list<int> li{1,2,3,4,5};
    li.remove(8);
    ploop<std::list<int>>(li);
    return 0;
}
