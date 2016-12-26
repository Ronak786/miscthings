#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "enclosure.h"


int main(void)
{
	struct enclosure enclosure[SLOTNUM];
	int j, res;

	if (get_enclosure_info(enclosure, SLOTNUM) == -1) {
		fprintf(stderr, "can not get enclosure\n"); 
		return -1;
	}

	printf("========================================\n");
	printf("||slot| devname |   size   |  status  ||\n");
	for (j = 0; j < SLOTNUM; j++) {
		printf("|| %-2d |  %-5s  | %6ldGB | %-8s ||\n",
			j, enclosure[j].devname, enclosure[j].size, 
			enclosure[j].state == OFFLINE ? "offline": (enclosure[j].state == ONLINE ? "running" : "empty"));
	}
	printf("=========================================\n");
	return 0;
}
