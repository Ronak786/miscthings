#include "duck.h"

namespace duck {
Duck::Duck(std::string name, FlyBehavior* fly, QuackBehavior* quack)
        : name_(name), flybehavior_(fly), quackbehavior_(quack) {
    std::cout << "initialized down for " + name << std::endl;
}

void Duck::display() {
    std::cout << "Duck's display" << std::endl;
}

MallardDuck::MallardDuck(std::string name, FlyBehavior* fly, QuackBehavior* quack) : Duck(name, fly, quack) {}
void MallardDuck::display() {
    std::cout << "MallardDuck's display" << std::endl;
}

RedheadDuck::RedheadDuck(std::string name, FlyBehavior* fly, QuackBehavior* quack) : Duck(name, fly, quack) {}
void RedheadDuck::display() {
    std::cout << "RedheadDuck's display" << std::endl;
}
} // namespace duck

int main() {
    duck::FlyNoWay noway;
    duck::FlyWithWings withwings;
    duck::Quack quack;
    duck::Squeak squeak;
    duck::MuteQuack mutequack;
    duck::MallardDuck mduck(std::string("mallard"), &noway, &squeak);
    duck::RedheadDuck rduck(std::string("redhead"), &withwings, &mutequack);
    duck::Duck &mduckref = mduck;
    duck::Duck &rduckref = rduck;
    mduckref.display();
    mduckref.PerformFly();
    mduckref.PerformQuack();
    rduckref.display();
    rduckref.PerformFly();
    rduckref.PerformQuack();
    return 0;
}
