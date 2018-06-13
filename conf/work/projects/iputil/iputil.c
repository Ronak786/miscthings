/*************************************************************************
	> File Name: iputil.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Wed 13 Jun 2018 04:04:59 PM CST
 ************************************************************************/

#include <stdlib.h>

int lte_open_modem() {
	system("ip addr add 10.5.74.231/32 dev seth_lte0");
	system("ip link set seth_lte0 up");
	system("ip route add default via 10.5.74.231 dev seth_lte0");
	return 0;
}

// divided into 5 level
int lte_get_siglevel() {
	return 4;
}
int lte_close_modem() {
	system("ip addr del 10.5.74.231/32 dev seth_lte0");
	system("ip link set seth_lte0 down");
	return 0;
}
