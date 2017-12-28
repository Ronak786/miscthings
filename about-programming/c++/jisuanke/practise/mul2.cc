#include <iostream>
#include <string>
#include <memory>

using namespace std;

struct A
{
    int data;

    A(int theData)
        :data{ theData }
    {
    }
};

struct B : virtual A
{
    B() :A{ 1 }
    {
        cout << data << endl;
    }
};

struct C : A
{
    C() :A{ 2 }
    {
        cout << data << endl;
    }
};

struct D : B, C
{
    D() :A{ 3 }
    {
    }
};

int main()
{
    D d;
    cout << d.data << endl;
    return 0;
}
