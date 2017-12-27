#include <iostream>
class Y
{
protected:
    int member;
};

class X : public Y
{
public:
    void Method(X& y)
    {
        &((y).member);
    }
};
int main() {}
