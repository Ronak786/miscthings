#include <stdinc.h>

int main(int ac, char *av[])
{
	int a, b, c;

	if (ac != 4)
	{
		printf("number 3\n");
		exit(1);
	}

	a = strtol(av[1], NULL, 10);
	b = strtol(av[2], NULL, 10);
	c = strtol(av[3], NULL, 10);

	printf("%s\n", (a == b && b == c) ? "equal" : "not equal");
	return 0;
}
