class A {
    private:
        A();
    public:
        ~A(){}
        A(int a) {};
};

class B:A {
    public:
        B(): A(2){}
};

int main() {
    B b;
    return 0;
}
