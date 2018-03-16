#include "stdio.h"

#include "analysis_package.h"
#include "serial.h"



int main(){
	int serialno = 0x0;
	int ret = serial_init("/dev/ttyMT3", 115200, 8, 'N', 1, 0, 0, 0);
	printf("serial_init ret = %d \n",ret);
	
	flush_uart();

	printf("++++++++++++++++ \n\n\n");
	
	ret = cmd_shankhand(serialno);
	printf("cmd_shankhand ret = %d \n\n\n",ret);

	serialno ++;

	ret = cmd_version(serialno);
	printf("cmd_version ret = %d \n\n\n",ret);

	serialno ++;

	ret = cmd_qbcode(serialno);
	printf("cmd_qbcode ret = %d \n\n\n",ret);
	
	close_uart();
	
	return 0;
}

