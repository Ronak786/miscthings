#include <stdinc.h>

int main(void)
{
	int f = 0;
	int g = 1;
	int i;

	for (i = 0; i <=15; i++)
	{
		printf("f %d\n", f);
		f = f + g;
		g = f - g;
	}

	return 0;
}
