#include <stdio.h>
#include <stdlib.h>

#include "enclosure.h"

int main(void)
{
	struct enclosure en[SLOTNUM];
	int i;

	if (get_enclosure_info(en) == -1) {
		fprintf(stderr, "can not get enclosure info\n");
		return -1;
	}

	for (i = 0; i < SLOTNUM; i++) {
		printf("slot %d, devname %s, state %d, size %luGB\n",
			i, en[i].devname, en[i].state, en[i].size);
	}
	return 0;
}
