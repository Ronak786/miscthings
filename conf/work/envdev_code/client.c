/*************************************************************************
	> File Name: /tmp/a.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Wed 30 May 2018 11:04:37 AM CST
 ************************************************************************/

#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

/*
 * input event use select:
 *		two dev: event0, event1
 *		
 * doto: three mode
 * for number: add into hashmap, every get, checktype and replace code
 * for alpha:  add into hashmap, every get,
 *				1, replace code, 
 *				2, set alarm function for a time interval
 *				3, get another and cancel existing alarm, 
 *					if previous alarm not finished && same code, 
 *						change code to the next%3, and alarm again
 *					if not same code, just start new alarm and send the last code, set last code to -1
 *					if alarm time ended, send code and set last to -1, means no last
 * for symbol:  just like number mode, one key on set
 *
 * mode change: use a last key current keycode is 28
 */


int main(int ac, char *av[]) {
	struct input_event ie;
	char * evname = "/dev/input/event3"; // we can test which is keyboard use method in qt qevdevkeyboardhandler.cpp
	char * fifoname = "/dev/event100";
	if (access(fifoname, F_OK) != 0) {
		int res = mkfifo(fifoname, 0666);
		if (res) {
			printf("can not create fifo\n");
			exit(0);
		}
	}

	int fdf = open(fifoname, O_WRONLY);
	int fdev = open(evname, O_RDONLY);

	if (fdf < 0 || fdev < 0) {
		printf("can not open fifo or event dev\n");
		exit(0);
	}
	
	// ignore signal pipe when read port is closed, occurred at write port
	signal(SIGPIPE, SIG_IGN);

	while (1) {
		if (read(fdev, &ie, sizeof(ie)) != sizeof(ie)) {
			printf("read error size from event dev\n");
		} else {
			printf("send type %d, code %d, value %d\n", ie.type, ie.code, ie.value);
			if (write(fdf, &ie, sizeof(ie)) != sizeof(ie)) {
				printf("write error into %s\n", fifoname);
			}
		}
	}
	return 0;
	/*
	signal(SIGPIPE, dumbhandle);
	struct input_event ie;
	unlink("/dev/event100");
	int res = mkfifo("/dev/event100", 0666);
	if (res < 0) {
		printf("can not create event100 or event0\n");
		exit(0);
	}
	int fdf = open("/dev/event100", O_WRONLY);
	if (fdf < 0) {
		printf("can not open fifo\n");
		exit(0);
	}

	ie.type=1;
	ie.code=2;
	ie.value=3;
	while (1) {
		printf("send type %d, code %d, value %d\n", ie.type, ie.code, ie.value);
		if (write(fdf, &ie, sizeof(ie)) != sizeof(ie)) {
			printf("write error in input/event0\n");
		}
		usleep(100000);
	}
	return 0;
	*/
}



