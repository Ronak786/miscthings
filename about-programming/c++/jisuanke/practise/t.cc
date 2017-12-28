#include <iostream>

using namespace std;

struct IX
{
    virtual ~IX() = default;

    virtual int GetX() = 0;
};

struct X : virtual IX
{
    int GetX() override
    {
        return 1;
    }
};

struct IY : IX
{
    virtual int GetY() = 0;
//    int GetX()override {return 20;}
};

/*
XY通过X::GetX来实现IY::GetX，这就是出现警告的原因。
原则上我们需要为X::GetX和IY::GetX都提供实现，但是这里面只有X::GetX不是纯虚函数，所以很符合我们对接口的要求。
我们知道这是对的，所以就临时禁用了这个警告。这样写的好处在于，如果有两个类都对同一个接口提供了实现，那总归会出现编译错误，这个编译指令也不解决这种情况，我们要亲自解决，以满足我们队接口的要求。
*/
#pragma warning(push)
#pragma warning(disable:4250)
struct XY : X, virtual IY
{
    int GetY() override
    {
        return 2;
    }
};
#pragma warning(pop)
int main() {
}
    
