#include "thread.h"
#include <iostream>
#include <cassert>

class A: public Mthread {
    private:
        int arg = 20;
    public:
        void start_routine() override {
            std::cout << "I get arg " << arg << std::endl;
        }
};

int main() {
    A md;
    md.run();
    assert(!md.hasError());
    md.join();
    assert(!md.hasError());
    return 0;
}
