#include <unistd.h>

#include <iostream>
#include <thread>

class hello {
    public:
        void h1(int a) {
            std::cout << "get " << a << std::endl;
        }
};

void hi(std::unique_ptr<int> ui) {
    std::cout << "ui is " << *ui << std::endl;
}

int main() {
    std::unique_ptr<int> p(new int(32));
    hi(std::move(p));
    return 0;
}
