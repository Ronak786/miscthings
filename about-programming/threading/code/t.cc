#include <thread>
#include <iostream>

class A {
    public:
        double operator()(int b) {
            std::cout << "inside b " << std::endl;
            return 30;
        }
        void func(int a) {
            std::cout << a << std::endl;
        }
};

int main() {
 //   std::thread t(&A::func, A(), 20);
    std::thread t2(A(), 20 );
//    t.join();
    t2.join();
    return 0;
}
