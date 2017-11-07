#include <unistd.h>

#include <iostream>
#include <thread>

class hello {
    public:
        void h1(int a) {
            std::cout << "get " << a << std::endl;
        }
};

int main() {
    hello h;
    std::thread t(&hello::h1, h, 20);
    std::cout << t.joinable() << std::endl;
    t.join();
    std::cout << t.joinable() << std::endl;
//    sleep(5);
    return 0;
}
