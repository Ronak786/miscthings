#include "../include/stdinc.h"

char *getword(FILE *fp, char *buf, int count)
{
        int ch;
	char *ptr = buf;

	if (count == 0) {
		fprintf(stderr, "buf count should > 0!!\n ");
		exit(1);
	}

        while (isspace(ch = getc(fp)))
                continue;

        while (count != 1)
        {
                if (ferror(fp) || feof(fp)) {
			*ptr = '\0';
                        return buf;
		}

		*ptr++ = ch;
		count--;
		if (isspace(ch = getc(fp)))
			break;
	}
	*ptr = '\0';
	return buf;
}

                 


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


	
	
		
	
