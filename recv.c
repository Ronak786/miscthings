#include   <stdio.h>     
#include   <stdlib.h>   
#include   <unistd.h>     
#include   <sys/types.h> 
#include   <sys/stat.h>  
#include   <fcntl.h>    
#include   <termios.h>  
#include   <errno.h>    
#include   <string.h>
#include   <signal.h>

#define TRUE 1


int right = 0 , wrong = 0;

void result( int num )
{
	printf(" answer is %d right and %d wrong\n", right, wrong);
	exit(0);

}
	



void setTermios(struct termios * pNewtio, int uBaudRate)
{
    bzero(pNewtio, sizeof(struct termios)); /* clear struct for new port settings */
    //8N1
    pNewtio->c_cflag = uBaudRate | CS8 | CREAD | CLOCAL;
    pNewtio->c_iflag = IGNPAR;
    pNewtio->c_oflag = 0;
    pNewtio->c_lflag = 0; //non ICANON
    /*
     initialize all control characters
     default values can be found in /usr/include/termios.h, and
     are given in the comments, but we don't need them here
     */
    pNewtio->c_cc[VINTR] = 0; /* Ctrl-c */
    pNewtio->c_cc[VQUIT] = 0; /* Ctrl-\ */
    pNewtio->c_cc[VERASE] = 0; /* del */
    pNewtio->c_cc[VKILL] = 0; /* @ */
    pNewtio->c_cc[VEOF] = 4; /* Ctrl-d */
    pNewtio->c_cc[VTIME] = 5; /* inter-character timer, timeout VTIME*0.1 */
    pNewtio->c_cc[VMIN] = 0; /* blocking read until VMIN character arrives */
    pNewtio->c_cc[VSWTC] = 0; /* '\0' */
    pNewtio->c_cc[VSTART] = 0; /* Ctrl-q */
    pNewtio->c_cc[VSTOP] = 0; /* Ctrl-s */
    pNewtio->c_cc[VSUSP] = 0; /* Ctrl-z */
    pNewtio->c_cc[VEOL] = 0; /* '\0' */
    pNewtio->c_cc[VREPRINT] = 0; /* Ctrl-r */
    pNewtio->c_cc[VDISCARD] = 0; /* Ctrl-u */
    pNewtio->c_cc[VWERASE] = 0; /* Ctrl-w */
    pNewtio->c_cc[VLNEXT] = 0; /* Ctrl-v */
    pNewtio->c_cc[VEOL2] = 0; /* '\0' */
}
 
#define BUFSIZE 7

int main(int argc, char **argv)
{
    int fd;
    int nread;
    char buff[BUFSIZE];
    struct termios oldtio, newtio;
    struct timeval tv;
    char buf[3] = {0x34, 0x30, 0x52};
    char *dev ="/dev/ttyS0";
    fd_set rfds; 
	int i = 0;
	int count = 0;
	
    if ((fd = open(dev, O_RDWR | O_NOCTTY))<0){		
		printf("err: can't open serial port!\n");
		return -1;
    }
	if (signal(SIGINT, result) == SIG_ERR)
	{
		printf(" signal load wrong\n");
		exit(1);
	}
    tcgetattr(fd, &oldtio); /* save current serial port settings */
    setTermios(&newtio, B38400);
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

	tv.tv_sec=30;
	tv.tv_usec=0;

	while (TRUE){
		printf("wait...\n");

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		if (select(1+fd, &rfds, NULL, NULL, NULL)>0){
			if (FD_ISSET(fd, &rfds)){
				nread=read(fd, buff, BUFSIZE);	//	printf("readlength=%d\n", nread);

				for (i = 0; i < nread; i++){
					printf ("0x%02x ", (unsigned char)buff[i]);
				}
				printf("\n");

				if (buff[nread - 1] == buf[count % 3]){
					printf ("0x%02x 0x%02x right\n", (unsigned char)buff[nread -1 ], (unsigned char)buf[count % 3]);
					right++;
				}
				else
				{
					printf ("0x%02x 0x%02x wrong\n", (unsigned char)buff[nread -1 ], (unsigned char)buf[count % 3]);
					wrong++;
				}
				count++;
			}
		}
	}

	tcsetattr(fd, TCSANOW, &oldtio);
	close(fd);
}
