#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "enclosure.h"

const char *optstring = "l";
struct opt {
	int list;
};

static int process_arg(struct opt *mopt, int ac, char *av[])
{
	int ch;
	while ((ch = getopt(ac, av, optstring)) != -1) {
		switch(ch) {
		case 'l':
			mopt->list = 1;
			break;
		default:
			fprintf(stderr, "unknown option: %c\n", ch);
			return -1;
		}
	}
	if (optind < ac) {
		fprintf(stderr, "too much args optind %d, argc %d\n", optind, ac);
		return -1;
	}
	return 0;
}
int main(int ac, char *av[])
{
	struct enclosure enclosure[SLOTNUM];
	int j, res;
	struct opt mopt;

	bzero(&mopt, sizeof(struct opt));
	if (-1 == process_arg(&mopt, ac, av)) {
		fprintf(stderr, "parse args error\n");
		exit(1);
	}
	if (get_enclosure_info(enclosure, SLOTNUM) == -1) {
		fprintf(stderr, "can not get enclosure\n"); 
		return -1;
	}

	if (mopt.list == 0) {
		printf("==============================================\n");
		for (j = SLOTNUM / 4 - 1; j >= 0; j--) {
			printf("||%8s | %8s | %8s | %8s ||\n",
				enclosure[j].state == OFFLINE ? "offline": (enclosure[j].state == ONLINE ? "running" : "empty"),
				enclosure[j+3].state == OFFLINE ? "offline": (enclosure[j+3].state == ONLINE ? "running" : "empty"),
				enclosure[j+6].state == OFFLINE ? "offline": (enclosure[j+6].state == ONLINE ? "running" : "empty"),
				enclosure[j+9].state == OFFLINE ? "offline": (enclosure[j+9].state == ONLINE ? "running" : "empty"));
		printf("==============================================\n");
		}
	} else 
		for ( j = 0; j < SLOTNUM; j++)
			printf("slot %2d, name %5s, status %8s\n", j, enclosure[j].devname,
				enclosure[j].state == OFFLINE ? "offline": (enclosure[j].state == ONLINE ? "running" : "empty"));
	return 0;
}
