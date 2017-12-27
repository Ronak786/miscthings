#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace std;

class Cat;
class Dog;
class Mouse;

class IAnimalVisitor {
    public:
    virtual void Visit(Cat * animal) = 0;
    virtual void Visit(Dog * animal) = 0;
    virtual void Visit(Mouse * animal) = 0;
};

class Animal
{
private:
    string name;

public:
    Animal(const string& theName)
        :name{ theName }
    {
    }

    virtual ~Animal()
    {
    }

    const string& GetName()const
    {
        return name;
    }

    virtual string Introduce()const = 0;
    
    virtual void Accept(IAnimalVisitor * visitor) = 0;

 
};

class Cat : public Animal
{
public:
    Cat(const string& theName)
        :Animal{ theName }
    {
    }

    string Introduce()const override
    {
        return "我是一只猫，我的名字叫\"" + GetName() + "\"。";
    }
    
    virtual void Accept(IAnimalVisitor * visitor) {
        visitor->Visit(this);
    }


};

class Dog : public Animal
{
public:
    Dog(const string& theName)
        :Animal{ theName }
    {
    }

    string Introduce()const override
    {
        return "我是一只狗，我的名字叫\"" + GetName() + "\"。";
    }
    
    virtual void Accept(IAnimalVisitor * visitor) {
        visitor->Visit(this);
    }
};

class Mouse : public Animal
{
public:
    Mouse(const string& theName)
        :Animal{ theName }
    {
    }

    string Introduce()const override
    {
        return "我是一只老鼠，我的名字叫\"" + GetName() + "\"。";
    }
    
    virtual void Accept(IAnimalVisitor * visitor) {
        visitor->Visit(this);
    }
};

int main()
{
    auto tom = make_shared<Cat>("Tom");
    auto jerry = make_shared<Mouse>("Jerry");
    auto spike = make_shared<Dog>("Spike");
    auto butch = make_shared<Cat>("Butch");
    auto lightning = make_shared<Cat>("Lightning");
    vector<shared_ptr<Animal>> friends{ tom, jerry, spike, butch, lightning };

    for (auto animal : friends)
    {
        animal->IntroduceForCat();
    }
    return 0;
}
