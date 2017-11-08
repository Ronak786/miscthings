#include <thread>
#include <iostream>

class A {
    public:
        double operator()(int b) {
            std::cout << "inside b " << std::endl;
            std::cout << "id is " << std::this_thread::get_id() << std::endl;
            return 30;
        }
        void func(int a) {
            std::cout << a << std::endl;
        }
};

int main() {
    std::cout << "master is " << std::this_thread::get_id() << std::endl;
    std::thread t1(&A::func, A(), 20);
    std::thread t2(A(), 30);
    std::cout << t1.get_id() << std::endl;
    std::cout << t2.get_id() << std::endl;
    t1.detach();
    std::cout << "after detach t1: " << t1.get_id() << std::endl;
    t2.detach();
    return 0;
}
