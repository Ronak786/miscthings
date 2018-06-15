#include <stdinc.h>

int main(void)
{
	int sum = 0;
	int i, j;

	for (i = 1; i < 1000; i*=2)
		for (j = 0; j < 1000; j++)
			sum ++;

	printf("sum is %d\n", sum);
	return 0;
}
	
