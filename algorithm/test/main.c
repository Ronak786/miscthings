#include <stdinc.h>
#include <getstring.h>

int main(void)
{
	char buf[20], *ptr;
	int count = 20;

	printf("we will get word from stdin\n");
	
	do {
		ptr = getword(stdin, buf, 20);
		if (*ptr != '\0')
			printf("get word %s\n", ptr);
	} while (*ptr != 0);

	printf("done\n");
	return 0;
}
	
	
