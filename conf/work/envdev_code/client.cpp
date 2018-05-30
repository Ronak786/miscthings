/*************************************************************************
	> File Name: /tmp/a.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Wed 30 May 2018 11:04:37 AM CST
 ************************************************************************/

#include <linux/input.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int ac, char *av[]) {
	int fd = open("/dev/input/event2", O_RDONLY);
	struct input_event ie;
	if (fd < 0) {
		printf("can not open event2\n");
		exit(0);
	}

	while (1) {
		if (read(fd, &ie, sizeof(ie)) != sizeof(ie)) {
			printf("read error size\n");
		} else {
			printf("get type %d, code %d, value %d\n", ie.type, ie.code, ie.value);
		}
	}
	return 0;
}



