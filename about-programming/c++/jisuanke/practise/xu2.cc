#include <iostream>

class Base
{
public:

    virtual void printMessage()
    {
        std::cout << "Base::printMessage()" << std::endl;
    }
};

class Derived : public Base
{
public:

    void printMessage()
    {
        std::cout << "Derived::printMessage()" << std::endl;
    }
};

int main(int argc, char* argv[])
{
    Derived d;

    unsigned int vtblAddress = *(unsigned int*)&d;

    typedef void(*pFun)(void*);

    pFun printFun = (pFun)(*(unsigned int*)(vtblAddress));

    printFun(&d);

    return 0;
}
