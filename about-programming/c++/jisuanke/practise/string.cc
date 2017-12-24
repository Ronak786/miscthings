#include <string>
#include <iterator>
#include <iostream>

int main() {
    std::string a = "abcde";
    a.replace(a.begin() + 2, a.begin() + 4, "bianmingliang");
    std::cout << a << std::endl;
    return 0;
}
