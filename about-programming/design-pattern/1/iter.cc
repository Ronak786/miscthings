#include "iter.h"

int main() {
    Iter it;
    it.AddItem("a");
    it.AddItem("b");
    it.AddItem("c");
    it.AddItem("d");
/*
    for (auto i = it.begin(); i != it.end(); ++i) {
        std::cout << *i << std::endl;
    }
*/
    for (auto i : it) 
        std::cout << i << std::endl;
    return 0;
}
        
