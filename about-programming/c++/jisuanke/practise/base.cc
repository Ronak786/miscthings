class Base1
{
private:
    int a1;

protected:
    int b1;

public:
    int c1;
};

class Base2
{
private:
    int a2;

protected:
    int b2;

public:
    int c2;
};

class Base3
{
private:
    int a3;

protected:
    int b3;

public:
    int c3;
};

class Y : public Base1, protected Base2, private Base3
{
};

class X : public Y
{
public:
    void Method(X& y) // 注意 y 的类型是 X
    {
        &y.a1; // 不可以
        &y.b1; // 可以
        &y.c1; // 可以

        &y.a2; // 不可以
        &y.b2; // 可以
        &y.c2; // 可以

        &y.a3; // 不可以
        &y.b3; // 不可以
        &y.c3; // 不可以
        
    }
};
