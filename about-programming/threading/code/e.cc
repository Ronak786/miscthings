#include <iostream>
#include <thread>
#include <ratio>
#include <chrono>

int main() {
    auto today = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto next = std::chrono::steady_clock::now();
    std::chrono::duration<double> dura = std::chrono::duration_cast<std::chrono::duration<double>>(next - today);
    std::cout << dura.count() << std::endl;
    std::cout << std::chrono::steady_clock::is_steady << std::endl;
    return 0;
}
