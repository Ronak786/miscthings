#include <stdinc.h>

char * l2s(long num, char *buf, int count)
{
	char *ptr = buf + count -1;
	
	*ptr-- = '\0';
	count--;
	
	do {
		*ptr-- = num % 2 + '0';
	} while ((num/=2) != 0 && --count);
	printf("count is %d\n", count);
	return ++ptr;
}


int main(int ac, char *av[])
{
	int count = 20, number;
	char buf[20];

	assert (ac != 1 );

	number = strtol(av[1], NULL, 10);

	printf("the number passed in is %s\n", l2s(number, buf, count));
	return 0;
}	
	

	
	
		
	
