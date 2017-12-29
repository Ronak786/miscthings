#include <iostream>
#include <string>
#include <memory>

using namespace std;

struct X
{
    virtual void Print() const
    {
        cout << "X" << endl;
    }
};

struct Y : X
{
    void Print() const override
    {
        cout << "Y" << endl;
    }
};

int main()
{
    const X& x = Y{};
    x.X::Print();

    (x.*&X::Print)();

    void (X::* print)()const = &X::Print;
    (x.*print)();

    return 0;
}
