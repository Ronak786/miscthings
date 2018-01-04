
#include <stdio.h>  
  
class A {
    int a;
    int b;
};

class B {
    int a;
    int b;
};
class C {
    int a;
    int b;
    class A {
        int a;
        int b;
};
    ~C();
};
class Data : virtual C
{  
public:  
    Data();  
    ~Data();  
};  
  
int main()  
{  
    printf("%lu",sizeof(Data));  
    return 0 ;
}  
