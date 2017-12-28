#include <iostream>
#include <string>
#include <memory>

using namespace std;

struct Animal
{
    virtual ~Animal() = default;
    virtual void PrintIntroduction() = 0;
};

struct Bear : virtual Animal
{
    void PrintIntroduction()override
    {
        cout << "我是一头熊。" << endl;
    }
};

struct Cat : virtual Animal
{
    void PrintIntroduction()override
    {
        cout << "我是一只猫。" << endl;
    }
};

struct BearCat: Bear, Cat {
    void PrintIntroduction() override
    { 
        cout << "I am a bearcat." << endl;
    }
};
int main()
{
    BearCat bearCat;
    bearCat.Bear::PrintIntroduction();
    return 0;
}
