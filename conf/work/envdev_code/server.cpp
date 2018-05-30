/*************************************************************************
	> File Name: /tmp/a.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Wed 30 May 2018 11:04:37 AM CST
 ************************************************************************/

#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <vector>
#include <map>

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
	struct input_event ie;
	char * evname = "/dev/input/event2"; // we can test which is keyboard use method in qt qevdevkeyboardhandler.cpp

    // creat fifo file if not exist
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


    handleKey();
	return 0;
}

void handleKey() {
    int mode = 0; // 0 number; 1 alpha; 2 symbol

    // map from keycode to keycode, we have three mode, use vector 
    std::vector<std::map<int, int>> mvec{std::map<int,int>, 
        std::map<int,int>, std::map<int,int>};
    initDict(mvec);

	while (1) {
		if (read(fdev, &ie, sizeof(ie)) != sizeof(ie)) {
			printf("read error size from event dev\n");
            continue;
		} 
        printf("get type %d, code %d, value %d from eventdev\n", ie.type, ie.code, ie.value);
        if (ie.type != EV_KEY) {
            // if not key event, just send thourgh
            if (write(fdf, &ie, sizeof(ie)) != sizeof(ie)) {
                printf("unmodified send: write error into %s\n", fifoname);
            }
        }
        switch (ie.code) {
            case KEY_LEFT:
                mode = (mode+1)%2; // current only number and alpha mode
                alarm(0); //cancel any alarm
                break;
            default:
                // only transfer recognized code
                if (mvec[mode].find(ie.code) != mvec[mode].end()) {
                    ie.code = vec[mode][ie.code];
                    
                    if (mode == 1 && (ie.code >= KEY_1 && ie.code <= KEY_7
                                || ie.code == KEY_NUMERIC_STAR
                                || ie.code == KEY_NUMERIC_POUND)) { // we have char change 
                        if (!samefield(lastcode, ie.code)) {
                            alarm(delaytime);
                            lastcode = ie.code;
                        } else {
                            int restime = alarm(delaytime);
                            if (restime != 0) { // we should change
                                changecurcode(ie.code); 
                                lastcode = ie.code;
                                sendbackspace(fdf);
                            }
                        }
                    }
                    if (write(fdf, &ie, sizeof(ie)) != sizeof(ie)) {
                        printf("modify send: write error into %s\n", fifoname);
                    }
                }
                break;
        }
		
	}
}

void initDict(std::vector<std::map<int,int>> &vec) {
    // for numerical
    // KEY_LEFT is special, used to switch mode
    // KEY_UP caps lock
    // KEY_ENTER use just as enter
    vec[0][KEY_1] = KEY_1; // KEY_0 is with 1
    vec[0][KEY_2] = KEY_2;
    vec[0][KEY_3] = KEY_3;
    vec[0][KEY_4] = KEY_4;
    vec[0][KEY_5] = KEY_5;
    vec[0][KEY_6] = KEY_6;
    vec[0][KEY_7] = KEY_7;
    vec[0][KEY_NUMERIC_STAR] = KEY_8;
    vec[0][KEY_NUMERIC_POUND] = KEY_9;
    vec[0][KEY_UP] = KEY_CAPSLOCK;
    vec[0][KEY_ENTER] = KEY_ENTER;

    vec[1][KEY_1] = KEY_A;
    vec[1][KEY_2] = KEY_D;
    vec[1][KEY_3] = KEY_G;
    vec[1][KEY_4] = KEY_J;
    vec[1][KEY_5] = KEY_M;
    vec[1][KEY_6] = KEY_P;
    vec[1][KEY_7] = KEY_S;
    vec[1][KEY_NUMERIC_STAR] = KEY_V;
    vec[1][KEY_NUMERIC_POUND] = KEY_Y;
    vec[1][KEY_UP] = KEY_CAPSLOCK;
}
