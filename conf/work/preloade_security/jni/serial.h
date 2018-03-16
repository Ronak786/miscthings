#ifndef _SERIAL_H
#define _SERIAL_H

int serial_init(char *dev,int speed,
	int databits,int parity,int stopbits,int hwf,int swf, int block);
int write_uart(unsigned char* buffer, int len);
int read_uart(unsigned char* buffer,int length,int timeout_ms);
int close_uart();
void flush_uart();

#endif
