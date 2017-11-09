#include <iostream>
#include <string>
#include <condition_variable>
#include <thread>
#include <mutex>

bool flag = false;
std::mutex mtx;
std::condition_variable cv;

void f1() {
    std::lock_guard<std::mutex> ld(mtx);
    std::cout << "I am in lock, ready to notify" << std::endl;
    flag = true;
    cv.notify_one();
}

void f2() {
    std::cout << "start to wait" << std::endl;
    std::unique_lock<std::mutex> ld(mtx);
    cv.wait(ld, []{ return flag; });
    std::cout << "I am in wake up" << std::endl;
    ld.unlock();
    std::cout << "I am in unlock" << std::endl;
}

int main() {
    std::thread t2(f2);
    std::thread t1(f1);
    t1.join();
    t2.join();
    return 0;
}

