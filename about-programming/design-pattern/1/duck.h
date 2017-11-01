#ifndef DUCK_H_
#define DUCK_H_

#include <iostream>
#include <string>


namespace duck {

//abstract class used to be inherited from
class FlyBehavior {
    public:
        virtual void fly() = 0;
};

class FlyWithWings : public FlyBehavior {
    public:
        void fly() { std::cout << "fly with wings" << std::endl; }
};

class FlyNoWay : public FlyBehavior {
    public:
        void fly() { std::cout << "fly no way" << std::endl;}
};

class QuackBehavior {
    public:
        virtual void quack() = 0;
};

class Quack : public QuackBehavior {
    public:
        void quack() { std::cout << "quack" << std::endl; }
};

class Squeak : public QuackBehavior {
    public:
        void quack() { std::cout << "squeak" << std::endl; }
};

class MuteQuack : public QuackBehavior {
    public:
        void quack() { std::cout << "mutequack" << std::endl; }
};

class Duck {
    private:
        std::string name_;
        FlyBehavior* flybehavior_;
        QuackBehavior* quackbehavior_;
    public:
        Duck(std::string name, FlyBehavior* fly, QuackBehavior* quack);
        //this method implemented by each subclass
        virtual void display() = 0;
        //below four methods shared by all subclasses
        void PerformQuack() { quackbehavior_->quack(); }
        void PerformFly() { flybehavior_->fly(); }
        void SetFlyBehavior(FlyBehavior* fly) {
            flybehavior_ = fly;
        }
        void SetQuackBehavior(QuackBehavior* quack) {
            quackbehavior_ = quack;
        }
};

class MallardDuck : public Duck {
    public:
        MallardDuck(std::string name, FlyBehavior* fly, QuackBehavior* quack);
        void display();
};

class RedheadDuck : public Duck {
    public:
        RedheadDuck(std::string name, FlyBehavior *fly, QuackBehavior *quack);
        void display();
};
}  //namespace duck 

#endif
