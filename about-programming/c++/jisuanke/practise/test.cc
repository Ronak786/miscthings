class A
{
public:
    int a;
};

struct B : A
{
};

int main()
{
    B b;
    b.a = 0;
    return 0;
}
