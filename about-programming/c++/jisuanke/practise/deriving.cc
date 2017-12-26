#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace std;

class Animal
{
private:
    string name;

public:
    Animal(const string& theName)
        :name{theName}
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
};

class Cat : public Animal
{
public:
    Cat(const string& theName)
        :Animal{theName}
    {
    }

    string Introduce()const override
    {
        return "我是一只猫，我的名字叫\"" + GetName() + "\"。";
    }
};

class Dog : public Animal
{
public:
    Dog(const string& theName)
        :Animal{theName}
    {
    }

    string Introduce()const override
    {
        return "我是一只狗，我的名字叫\"" + GetName() + "\"。";
    }
};

class Mouse : public Animal
{
public:
    Mouse(const string& theName)
        :Animal{theName}
    {
    }

    string Introduce()const override
    {
        return "我是一只老鼠，我的名字叫\"" + GetName() + "\"。";
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
        cout << animal->Introduce() << endl;
    }
    return 0;
}
