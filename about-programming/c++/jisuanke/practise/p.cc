#include <iostream>
#include <string>
#include <memory>

using namespace std;

struct X
{
    void A()
    {
    }

    int B(string u, double v)
    {
        return {};
    }

    string C(bool b)const
    {
        return{};
    }

    string D(bool b)&&
    {
        return{};
    }
};

int main()
{

    void(X::* a)() = &X::A;
    int(X::* b)(string, double)= &X::B;
    string(X::* c)(bool)const= &X::C;
    string(X::* d)(bool)&&= &X::D;

    return 0;
}
