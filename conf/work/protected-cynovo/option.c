
#include "option.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include "hlog.h"
#include "analysis_module.h"

char comdev[64] = {0};
char comarg[64] = {0};
int sockettype = 0;
char socketaddr[64] = {0};
int daemonflag = 0;
int testflag = 0;

struct option opts[] = 
{
    {"debug",  		no_argument, 		NULL, 'd'},
	{"daemon",  	no_argument, 		NULL, 'm'},
	{"comdev",  	required_argument, 	NULL, 'e'},
	{"comarg",  	required_argument, 	NULL, 'a'},
	{"sockettype",  required_argument, 	NULL, 'y'},
	{"socketaddr",  required_argument, 	NULL, 'r'},
	{"test",    	no_argument,       	NULL, 't'},
    {"help",    	no_argument,       	NULL, 'h'},
    {"version", 	no_argument,       	NULL, 'v'} 
};
 
static void usage()
{
	printf("Usage: ./payment-service -d \n");
	printf("-d --debug \n");
	printf("-m --daemon \n");
	printf("-e --comdev /dev/ttyS0\n");
	printf("-a --comarg \n");
	printf("-y --sockettype [tcp|udp|local]\n");
	printf("-r --socketaddr [port|local_file]\n");
	printf("-d --debug \n");
	printf("-v --version \n");
	printf("-h --help \n");
}

int option(int argc, char **argv)
{
	int ret = 0;
	int opt = 0; 
	
	while((opt = getopt_long(argc, argv, "dme:a:y:r:thv", opts, NULL)) != -1)
	{
		switch(opt)
		{
			case 'e':
				strncpy(comdev, optarg, sizeof(comdev));
				printf("payment-service comdev = %s\n", comdev);
				break;
				break;
			case 'a':
				strncpy(comarg, optarg, sizeof(comarg));
				printf("payment-service comarg = %s\n", comarg);
				break;
			case 'y':
				if(!strcasecmp(optarg, "local"))
					sockettype = SOCKET_TYPE_LOCAL;
				else if(!strcasecmp(optarg, "udp"))
					sockettype = SOCKET_TYPE_UDP;
				else if(!strcasecmp(optarg, "tcp"))
					sockettype = SOCKET_TYPE_TCP;
				else
				{
					printf("\"%s\" is invalid\n", optarg);
					ret = -1;
					break;
				}	
				printf("payment-service sockettype = %s\n", optarg);
				break;
			case 'r':
				strncpy(socketaddr, optarg, sizeof(comarg));
				printf("payment-service socketaddr = %s\n", socketaddr);
				break;
			case 'd':
				hlog_set_level("debug");
				break;
			case 'm':
				daemonflag = 1;
				printf("daemon mode\n");
				break;
			case 't':
				testflag = 1;
				printf("test mode\n");
				break;
			case 'v':
				printf("payment-service version %s\n", VERSION);
				ret = -1;
				break;
			case 'h':
				usage();
				ret = -1;
				break;
			default:
				ret = -1;
				break;
		}
	}
	
	return ret;
}
