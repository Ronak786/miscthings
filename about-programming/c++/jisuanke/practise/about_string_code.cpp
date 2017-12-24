#include <iostream>
#include <string>
#include <regex>

int main() {
    std::string line;
//    std::string regstring = R"(\w+@(\w+\.)+\w+)";
//    std::string regstring = R"(abc)";
//    std::regex reg(R"(\w+@(\w+\.)+\w+)");
//    std::regex reg("\\w+@(\\w+\\.)+\\w+");

    while (std::getline(std::cin, line)) {
//            std::cout << "got line " << line << std::endl;
            std::smatch sm;
            std::regex_search(line, sm, reg);
            std::cout << "result is " << sm.str() << std::endl;
    }
    return 0;
}
