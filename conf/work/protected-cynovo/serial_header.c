#include "stdio.h"
#include "protect_cynovo.h"
#include "serial_header.h"
#include <cutils/properties.h>
#include "hlog.h"

const int page_serialnoheader = 10;

void init_serial_header()
{

	unsigned char serialheader[MAXLENGTH] = {0};
	int ret = read_serial_header(serialheader);
	if(ret>0 && ret <32){
		serialheader[ret] = 0;
		property_set("ro.serialno.header", serialheader);
	}
}

int read_serial_header(unsigned char  *buf)
{
	int ret = open_disk();
	if(ret<0)
		goto out;
	
	ret = read_pages(page_serialnoheader,buf);
	
	close_disk();
out:
	return ret;

}
