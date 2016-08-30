#include <stdio.h>
#include <stdlib.h>

static int gcd(int a, int b);
int main(int ac, char *av[])
{
	int a, b;

	if (ac != 3) {
		printf("wrong input number, must be three\n");
		exit(1);
	}

	a = strtol(av[1], NULL, 10);
	b = strtol(av[2], NULL, 10);
	printf("gcd of a and b is %d\n", gcd(a, b));
	return 0;
}



static int gcd(int a, int b)
{
	int r;

	if (b == 0)
		return a;
	r = a % b;
	return gcd(b, r);
}
	
