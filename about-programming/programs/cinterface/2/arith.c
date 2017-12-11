#include "arith.h"

/*
 * benefit: when used in hash, the Arith_mod always return 
 * a positive number between 0 and N-1, if we use x %2
 */
int Arith_max(int x, int y) {
    return x > y ? x : y;
}

int Arith_min(int x, int y) {
    return x < y ? x : y;
}

int Arith_div(int x, int y) {
    if (-13 / 5 == -2 
            && (x < 0) != (y < 0)
            && x % y != 0) {
        return x / y - 1;
    } else {
        return x / y;
    }
}

int Arith_mod(int x, int y) {
    return x - y * Arith_div(x, y);
}

int Arith_floor(int x, int y) {
    return Arith_div(x, y);
}

int Arith_ceiling(int x, int y) {
    return Arith_floor(x, y) + (x%y != 0);
}
