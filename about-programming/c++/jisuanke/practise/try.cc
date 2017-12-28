#include <iostream>
#include <string>
#include <memory>

using namespace std;

struct A
{
    int a;

    virtual ~A() = default;

    void Print()
    {
        cout << a << endl;
    }
};

struct B
{
    int b;

    virtual ~B() = default;

    void Print()
    {
        cout << b << endl;
    }
};

struct C : A, B
{
};

int main()
{
    C c;
    c.a = 1;
    c.b = 2;

    B* pb = reinterpret_cast<B*>(&c);
    pb->Print();
    return 0;
}
