/*************************************************************************
	> File Name: /tmp/a.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Wed 30 May 2018 11:04:37 AM CST
 ************************************************************************/

#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <vector>
#include <map>

#define KEY_CUSTOM_UNUSED 254

bool samefield(const __u16 c1, const __u16 c2);
__u16 changecurcode(__u16 code);
void initDict(std::vector<std::map<__u16,__u16>> &vec, std::map<__u16,__u16> &sec);
void handleKey(std::vector<const char*>, int fdev);
void sendbackspace(int fdf);
void dumbalarm(int sig);

static std::map<__u16,__u16> sec;

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

// test, we use 

int main(int ac, char *av[]) {
	const char * evname = "/dev/input/event2"; // we can test which is keyboard use method in qt qevdevkeyboardhandler.cpp
	// used in x86 laptop test
//	const char * evname = "/dev/input/event3"; // we can test which is keyboard use method in qt qevdevkeyboardhandler.cpp
	const char * fifoname = "/dev/event100";

	std::vector<const char*> fdvec; // currently only test one
	if (ac == 1) {
		fdvec.push_back(evname);
	} else {
		for(int i = 1; i < ac; ++i) {
			fdvec.push_back(av[i]);
		}
	}

	daemon(0, 0); // daemon one self

    // creat fifo file if not exist
	if (access(fifoname, F_OK) != 0) {
		int res = mkfifo(fifoname, 0666);
		if (res) {
			printf("can not create fifo\n");
			exit(0);
		}
	}

	int fdf = open(fifoname, O_WRONLY);

	if (fdf < 0) {
		printf("can not open fifo\n");
		exit(0);
	}
	printf("open pip success\n");
	
	// ignore signal pipe when read port is closed, occurred at write port
	signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, dumbalarm);

    handleKey(fdvec, fdf);
    close(fdf);
	return 0;
}

void handleKey(std::vector<const char*> fdvec, int fdf) {
	struct input_event ie;
    int mode = 0; // 0 number; 1 alpha; 2 symbol
    __u16 lastcode = (__u16)-1;
	__u16 lastpush = (__u16)-1 -1;
    int delaytime = 1;

	fd_set sset, backset;
	FD_ZERO(&sset);
	int maxfd = -1;
	for (auto name: fdvec) {
		int fd = open(name, O_RDONLY);
		if (fd >= 0) {
			FD_SET(fd, &sset);
			maxfd = fd > maxfd ? fd : maxfd;
		} else {
			printf("can not open eventdev %s\n", name);
		}
	}

	if (maxfd < 0) {
		printf("not find any event input, stop\n");
		return;
	} else {
		printf("max fd is %d\n", maxfd);
	}

    // map from keycode to keycode, we have three mode, use vector 
    std::vector<std::map<__u16, __u16>> mvec{std::map<__u16,__u16>(), std::map<__u16,__u16>(), std::map<__u16,__u16>()};

    initDict(mvec, sec);

	backset = sset;
	while (1) {
		sset = backset;
		int res = select(maxfd+1, &sset, NULL, NULL, NULL);
		if (res > 0) {
			for (int i = 0; i < maxfd+1; ++i) {
				if( FD_ISSET(i, &sset)) {
					if (read(i, &ie, sizeof(ie)) != sizeof(ie)) {
						printf("read error size from event dev\n");
						continue;
					} 
//					printf("get type %d, code %d, value %d from eventdev\n", ie.type, ie.code, ie.value);
					if (ie.type != EV_KEY) { // if not keycode or is pop
						// if not key event, just send thourgh
						if (write(fdf, &ie, sizeof(ie)) != sizeof(ie)) {
							printf("unmodified send: write error into fifo\n");
						}
						continue;
					}

					switch (ie.code) {
						case KEY_F1:
							if (ie.value == 0) { // handle model change after key pop
								mode = (mode+1)%3; // current only number and alpha mode and navigation mode
								alarm(0); //cancel any alarm
								lastcode = (__u16)-1;
								printf("changing mode to %d\n", mode);
							}
							break;
						default:
							// only transfer recognized code
							if (mvec[mode].find(ie.code) != mvec[mode].end()) {
								
								if (ie.value == 0) { // pop
									/*
									if (lastcode != (__u16)-1) {
										ie.code = lastcode;
									}
									*/
									ie.code = lastpush;
									if (write(fdf, &ie, sizeof(ie)) != sizeof(ie)) {
										printf("pop send: write error into fifo\n");
									}
									break;
								}
									
								// handle mode 1's alpha transform
								if (mode == 1 && (ie.code >= KEY_1 && ie.code <= KEY_9)) { // we have char change 
									ie.code = mvec[mode][ie.code];
									if (!samefield(lastcode, ie.code)) {
										alarm(delaytime);
										lastcode = ie.code;
									} else {
										int restime = alarm(delaytime);
										if (restime != 0) { // we should change
											ie.code = changecurcode(lastcode); 
											sendbackspace(fdf);
										}
										lastcode = ie.code;
									}
								} else {
									ie.code = mvec[mode][ie.code];
								}
								lastpush = ie.code;
								if (write(fdf, &ie, sizeof(ie)) != sizeof(ie)) {
									printf("modify send: write error into fifo\n");
								}
							}
							break;
					}
				}
			}
		}else {
			perror("some error while select: ");
		}
	}
}

void initDict(std::vector<std::map<__u16,__u16>> &vec, std::map<__u16,__u16> &sec) {
    // for numerical
    // KEY_F1 is special, used to switch mode
    // KEY_0 caps lock
    // KEY_ENTER use just as enter
    vec[0][KEY_1] = KEY_1; // KEY_0 is with 1
    vec[0][KEY_2] = KEY_2;
    vec[0][KEY_3] = KEY_3;
    vec[0][KEY_4] = KEY_4;
    vec[0][KEY_5] = KEY_5;
    vec[0][KEY_6] = KEY_6;
    vec[0][KEY_7] = KEY_7;
    vec[0][KEY_8] = KEY_8;
    vec[0][KEY_9] = KEY_9;
    vec[0][KEY_0] = KEY_0;
    vec[0][KEY_DELETE] = KEY_BACKSPACE;
	vec[0][KEY_ESC] = KEY_ESC;
	vec[0][KEY_SPACE] = KEY_ENTER;

    vec[1][KEY_1] = KEY_A;
    vec[1][KEY_2] = KEY_D;
    vec[1][KEY_3] = KEY_G;
    vec[1][KEY_4] = KEY_J;
    vec[1][KEY_5] = KEY_M;
    vec[1][KEY_6] = KEY_P;
    vec[1][KEY_7] = KEY_S;
    vec[1][KEY_8] = KEY_V;
    vec[1][KEY_9] = KEY_Y;
    vec[1][KEY_0] = KEY_CAPSLOCK;
    vec[1][KEY_DELETE] = KEY_BACKSPACE;
	vec[1][KEY_ESC] = KEY_ESC;
	vec[1][KEY_SPACE] = KEY_ENTER;
    
	// navigation key
//    vec[2][KEY_1] = KEY_CUSTOM_UNUSED;
    vec[2][KEY_2] = KEY_UP;
//    vec[2][KEY_3] = KEY_CUSTOM_UNUSED;
    vec[2][KEY_4] = KEY_LEFT;
//    vec[2][KEY_5] = KEY_CUSTOM_UNUSED;
    vec[2][KEY_6] = KEY_RIGHT;
//    vec[2][KEY_7] = KEY_CUSTOM_UNUSED;
    vec[2][KEY_8] = KEY_DOWN;
//    vec[2][KEY_9] = KEY_CUSTOM_UNUSED;
//    vec[2][KEY_0] = KEY_CUSTOM_UNUSED;
    vec[2][KEY_DELETE] = KEY_BACKSPACE;
	vec[2][KEY_ESC] = KEY_ESC;
	vec[2][KEY_SPACE] = KEY_ENTER;

    sec[KEY_A] = KEY_B;
    sec[KEY_B] = KEY_C;
    sec[KEY_C] = KEY_A;
    sec[KEY_D] = KEY_E;
    sec[KEY_E] = KEY_F;
    sec[KEY_F] = KEY_D;
    sec[KEY_G] = KEY_H;
    sec[KEY_H] = KEY_I;
    sec[KEY_I] = KEY_G;
    sec[KEY_J] = KEY_K;
    sec[KEY_K] = KEY_L;
    sec[KEY_L] = KEY_J;
    sec[KEY_M] = KEY_N;
    sec[KEY_N] = KEY_O;
    sec[KEY_O] = KEY_M;
    sec[KEY_P] = KEY_Q;
    sec[KEY_Q] = KEY_R;
    sec[KEY_R] = KEY_P;
    sec[KEY_S] = KEY_T;
    sec[KEY_T] = KEY_U;
    sec[KEY_U] = KEY_S;
    sec[KEY_V] = KEY_W;
    sec[KEY_W] = KEY_X;
    sec[KEY_X] = KEY_V;
    sec[KEY_Y] = KEY_Z;
    sec[KEY_Z] = KEY_Y;
                     

}


// check whether c1 and c2 belong to the same button on phone kbd
bool samefield(const __u16 c1, const __u16 c2) {
    bool status = false;
    if (c1 == KEY_A || c1 == KEY_B || c1 == KEY_C) 
        status = (c2 == KEY_A || c2 == KEY_B || c2 == KEY_C);
    else if (c1 == KEY_D || c1 == KEY_E || c1 == KEY_F) 
        status = (c2 == KEY_D || c2 == KEY_E || c2 == KEY_F);
    else if (c1 == KEY_G || c1 == KEY_H || c1 == KEY_I) 
        status = (c2 == KEY_G || c2 == KEY_H || c2 == KEY_I);
    else if (c1 == KEY_J || c1 == KEY_K || c1 == KEY_L) 
        status = (c2 == KEY_J || c2 == KEY_K || c2 == KEY_L);
    else if (c1 == KEY_M || c1 == KEY_N || c1 == KEY_O) 
        status = (c2 == KEY_M || c2 == KEY_N || c2 == KEY_O);
    else if (c1 == KEY_P || c1 == KEY_Q || c1 == KEY_R) 
        status = (c2 == KEY_P || c2 == KEY_Q || c2 == KEY_R);
    else if (c1 == KEY_S || c1 == KEY_T || c1 == KEY_U) 
        status = (c2 == KEY_S || c2 == KEY_T || c2 == KEY_U);
    else if (c1 == KEY_V || c1 == KEY_W || c1 == KEY_X) 
        status = (c2 == KEY_V || c2 == KEY_W || c2 == KEY_X);
    else if (c1 == KEY_Y || c1 == KEY_Z) 
        status = (c2 == KEY_Y || c2 == KEY_Z);
    else  { 
        printf("we reach here as c1 or c2 belongs to not a-z %d %d\n", c1, c2);
        status = false;
    }
    return status;
}


// change code to neighbour code
__u16 changecurcode(__u16 code) {
    return sec[code];    

}

void sendbackspace(int fdf) {
    struct input_event ie;
    ie.type = EV_KEY;
    ie.code = KEY_BACKSPACE;
    ie.value = 1;

    if (write(fdf, &ie, sizeof(ie)) != sizeof(ie)) {
        printf("backspace push: write error into fifoname\n");
    }
    ie.value = 0;
    if (write(fdf, &ie, sizeof(ie)) != sizeof(ie)) {
        printf("backspace pop: write error into fifoname\n");
    }
}


void dumbalarm(int sig) {}
