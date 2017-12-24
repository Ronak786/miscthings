#include <iostream>

using namespace std;

struct Exception
{
};

struct Throw
{
    int id;

    Throw(int theId, bool requireThrow)
        :id{ theId }
    {
        cout << "Throw(" << id << ")" << endl;
        if (requireThrow)
        {
            throw Exception{};
        }
    }

    ~Throw()
    {
        cout << "~Throw(" << id << ")" << endl;
    }
};

struct Bad
{
    Throw throw1;
    Throw throw2;
    Throw throw3;

    Bad(int id)
        :throw1{ 1,id == 1 }
        , throw2{ 2,id == 2 }
        , throw3{ 3,id == 3 }
    {
        cout << "Bad()" << endl;
        throw Exception{};
    }

    ~Bad()
    {
        cout << "~Bad()" << endl;
    }
};

int main(){
    Bad{1};
    return 0;
}
