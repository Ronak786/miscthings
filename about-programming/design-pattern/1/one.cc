#include <iostream>

class A {
    private:
        static A *instance;
        A(int a = 1) { std::cout << "create" << std::endl; }
    public:
        static A *getInstance() {
            if (instance == nullptr)
                instance = new A(1);
            return instance;
        }
        void get() { std::cout << 20 << std::endl; }
};
A* A::instance = nullptr;

int main() {
    A *a(A::getInstance());
    a->get();
    return 0;
}
