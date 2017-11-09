#include <future>
#include <iostream>
#include <thread>
#include <functional>

void f1(std::future<int> & fut) {
    std::cout << "future is " << fut.get() << std::endl;
}

int main() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();
    std::thread t1(f1, std::ref(fut));
    std::cout << "waiting " << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    prom.set_value(20);
    t1.join();
    return 0;
}
    
