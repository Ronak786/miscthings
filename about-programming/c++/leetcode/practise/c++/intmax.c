#include <stdio.h>
#include <stdint.h>

int main() {
    int x = 964632435;
    int y = INT32_MAX;
    printf ("%d %x\n", y, y);
    printf ("%d %x\n", x, x);
    printf ("%d %x\n", x*10, x*10);
    return 0;
}
