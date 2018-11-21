#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define SIZE 303

int main(int ac, char *av[]) {
	int counts = 1;
	int width = 300;
	int interval = 1;
	int time;
	int wrong = 0;
	int total = 0;
	int start = 0;
	int height = 0;
	int frame_head = 0;
	int print_bytes = 32;
	int tail = 0;
	if (ac == 9) {
		width = atoi(av[1]);
//		width += 8; // for frame head
		height = atoi(av[2]);
		frame_head = atoi(av[3]);
		interval = atoi(av[4]); // print every interval times read
		counts = atoi(av[5]); // read count times every second
		total = atoi(av[6]); // total read times
		print_bytes = atoi(av[7]);
		tail = atoi(av[8]);
	} else {
		printf("args width, height, framehead, interval, freq, total, print_bytes, tail\n");
		return -1;
	}
	int totalbytes = width * height + frame_head;

	if (counts != 0) {
		time = 1000000 / counts;
	} else {
		time = 0;
	}
	printf("width: %d, height %d,  frame_head %d, count %d ,time : %d\n", width, height, frame_head, counts, time);

	int fd = open("/dev/cam", O_RDWR);
	if (fd == -1) {
		printf("can not open\n");
		return 0;
	}

	unsigned char *buf = (unsigned char*)malloc(totalbytes*sizeof(char));
	if (!buf) {
		printf("can not allocate buffer\n");
		return -1;
	}

	int count = 0;
	int ret;
	while (count < total) {
		if (time) {
			usleep(time);
		}
		++count;
		memset(buf, 0, totalbytes);
		if ((ret = read(fd, buf, totalbytes)) != totalbytes) {
			printf(" read error at count %d, error: %d\n", count, ret);
			continue;
		}

		// show every interval times
		if (count % interval == 0) {
			printf("count: %d\n", count);
			{
				int i = 0;
				for (i = 0; i < print_bytes; ++i) {
					printf("%02x ", buf[i]);
					if (i % 32 == 31) {
						putchar('\n');
					}
				}
				if (tail != 0) {
					for (i = print_bytes; i > 0; --i) {
						printf("%02x ", buf[totalbytes - i]);
						if (i % 32 == 31) {
							putchar('\n');
						}
					}
				}
			}
			printf("\n\n");
		}
	}

//	printf("wrong %d, count %d, rate %f\n", wrong, count, wrong *1.0 / count);
	free(buf);
	close (fd);
	return 0;
}
