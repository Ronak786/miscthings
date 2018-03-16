
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include "serial.h"

static int SerialPrinterHandle = 0;

static int serial_config(int speed,int databits,int parity,int stopbits,int hwf,int swf)
{
        int i, ret;
        struct termios tty;
        int speed_arr[] = {B230400, B115200, B57600, B38400, B19200,
            B9600, B4800, B2400, B1200, B300,B0
        };
        int name_arr[] = {230400, 115200, 57600, 38400,  19200,
            9600, 4800, 2400, 1200,  300, 0
        };
        ret = tcgetattr(SerialPrinterHandle, &tty);
        if (ret <0)
        {
            //OSTOOLS_Printf("tcgetattr error\n");
            close(SerialPrinterHandle);
            return -2;
        }
        //???
        for (i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
        {
            if (speed == name_arr[i])
            {
                cfsetispeed(&tty, speed_arr[i]);
                cfsetospeed(&tty, speed_arr[i]);
                break;
            }
            if (name_arr[i] == 0)
            {
                //OSTOOLS_Printf("speed %d is not support,set to 9600\n",speed);
                cfsetispeed(&tty, B9600);
                cfsetospeed(&tty, B9600);
            }
        }
        //???
        switch (databits)
        {
        case 5: tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS5; break;
        case 6: tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS6; break;
        case 7: tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS7; break;
        case 8:
        default: tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; break;
        }
        //???
        if (stopbits == 2)
            tty.c_cflag |= CSTOPB;
        else
            tty.c_cflag &= ~CSTOPB;
        //????
        switch (parity)
        {
        //?????
        case 'n':
        case 'N':
            tty.c_cflag &= ~PARENB;   /* Clear parity enable */
            tty.c_iflag &= ~INPCK;    /* Enable parity checking */
            break;
        //???
        case 'o':
        case 'O':
            tty.c_cflag |= (PARODD | PARENB); /* ??????*/
            tty.c_iflag |= INPCK;     /* Disable parity checking */
            break;
        //???
        case 'e':
        case 'E':
            tty.c_cflag |= PARENB;    /* Enable parity */
            tty.c_cflag &= ~PARODD;   /* ??????*/
            tty.c_iflag |= INPCK;     /* Disable parity checking */
            break;
        //??????????
        case 'S':
        case 's':  /*as no parity*/
            tty.c_cflag &= ~PARENB;
            tty.c_cflag &= ~CSTOPB;
            break;
        default:
            tty.c_cflag &= ~PARENB;   /* Clear parity enable */
            tty.c_iflag &= ~INPCK;    /* Enable parity checking */
            break;
        }
        //?????
        if (hwf)
            tty.c_cflag |= CRTSCTS;
        else
            tty.c_cflag &= ~CRTSCTS;
        //?????
        if (swf)
            tty.c_iflag |= IXON | IXOFF;
        else
            tty.c_iflag &= ~(IXON|IXOFF|IXANY);

        //???RAW??
        tty.c_iflag &= ~(IGNBRK | IGNCR | INLCR | ICRNL | IUCLC |
             IXANY | IXON | IXOFF | INPCK | ISTRIP);
        tty.c_iflag |= (BRKINT | IGNPAR);
        tty.c_oflag &= ~OPOST;
        tty.c_lflag &= ~(ECHONL|NOFLSH);//tty.c_lflag &= ~(XCASE|ECHONL|NOFLSH);
        tty.c_lflag &= ~(ICANON | ISIG | ECHO);
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cc[VTIME] = 5;
        tty.c_cc[VMIN] = 1;

        ret = tcsetattr(SerialPrinterHandle, TCSANOW, &tty);

        return ret;
}


static int readu(unsigned char* buffer, int timeout_ms,int length)
{
	int nfds;
	fd_set readfds;
	struct timeval	tv;

	tv.tv_sec = timeout_ms / (1000*1000);
	tv.tv_usec = timeout_ms % (1000*1000);

	FD_ZERO(&readfds);
	FD_SET(SerialPrinterHandle, &readfds);
	nfds = select(SerialPrinterHandle+1, &readfds, NULL, NULL, &tv);
	if (nfds <= 0) {
		return(-1);
	}

 	return  read(SerialPrinterHandle, buffer, length);
}

static int read_uart_noblock(unsigned char* buffer, int timeout_ms,int length,int times)
{
	int i =0;
	int alreadlen = 0;
	int ret =0;

	for(i=0;i<times;i++){
		ret = readu(buffer+alreadlen,timeout_ms,length-alreadlen);
		if(ret>0)
			alreadlen = alreadlen + ret;
		if(alreadlen>=length)
			break;
	}
	return alreadlen;
}


int serial_init(char *dev,int speed,int databits,int parity,int stopbits,int hwf,int swf, int block)
{
        int ret;
        if( block)
            SerialPrinterHandle = open(dev,O_RDWR|O_NOCTTY);
        else
            SerialPrinterHandle = open(dev,O_RDWR|O_NOCTTY|O_NONBLOCK);

        if( !SerialPrinterHandle)
        {
            //OSLOG(L_WARNING, "cannot open %s\n",dev);
            return -1;
        }

        ret = serial_config(speed,databits,parity,stopbits,hwf,swf);
        if (ret < 0)
        {
             close(SerialPrinterHandle);
             return -3;
        }
        tcflush(SerialPrinterHandle,TCIOFLUSH);
        return SerialPrinterHandle;
}


int write_uart(unsigned char* buffer, int len)
{
	int retval = 0;
 	return write(SerialPrinterHandle, buffer, len);
}

int read_uart(unsigned char* buffer,int length,int timeout_ms)
{
	int times = timeout_ms/(20*1000) + 1;

	int ret = read_uart_noblock(buffer,(20*1000) ,length,times);

	return ret;	
}

int close_uart()
{
    close(SerialPrinterHandle);
}

void flush_uart()
{
    tcflush(SerialPrinterHandle,TCIOFLUSH);
}


