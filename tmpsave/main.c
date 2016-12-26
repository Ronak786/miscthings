#include <stdio.h>

#include "enclosure.h"


int main(void)
{
	struct enclosure enclosure[SLOTNUM];
	int j;

	if (get_enclosure_info(enclosure, SLOTNUM) == -1) {
		fprintf(stderr, "can not get enclosure\n");
		return -1;
	}
	for (j = 0; j < SLOTNUM; j++) {
		printf("slot %d, devname %s, size %ldgb, state %d\n",
			j, enclosure[j].devname, enclosure[j].size, enclosure[j].state);
	}
	return 0;
}
