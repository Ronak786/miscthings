#include "coffee.h"

namespace drink {
Beverage::Beverage(double base) : base_item_price_(base){}
double Beverage::Cost() { 
    return base_item_price_ + size_; }
HouseBlend::HouseBlend(double drink, double base) :
        drink_price_(drink), Beverage(base) {}
double HouseBlend::Cost() { return drink_price_ + Beverage::Cost(); }
DarkRoast::DarkRoast(double drink, double base) :
        drink_price_(drink), Beverage(base) {}
double DarkRoast::Cost() { return drink_price_ + Beverage::Cost(); }
Milk::Milk(double price, Beverage * base) :
    milk_price_(price), bptr_(base) {}
Moncha::Moncha(double price, Beverage* base): 
    moncha_price_(price), bptr_(base) {}
}  //namespace drink

int main() {
    drink::HouseBlend house(20, 10);
    drink::DarkRoast dark(40, 10);
    std::cout << house.Cost() << std::endl;
    std::cout << dark.Cost() << std::endl;
    drink::Milk milk(33, &house);
    drink::Moncha moncha(24, &milk);
    std::cout << milk.Cost() << std::endl;
    std::cout << moncha.Cost() << std::endl;
    moncha.SetSize(22);
    std::cout << moncha.Cost()  << std::endl;
    return 0;
}

