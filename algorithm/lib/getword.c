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

                 

