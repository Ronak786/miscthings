class A {
    private:
        A() = default;
    public:
        int b;
        virtual ~A()  = default;
};

class B : public A {
};

int main() {
    B b;
}
