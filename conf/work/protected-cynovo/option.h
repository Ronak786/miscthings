
#ifndef _OPTION_H
#define _OPTION_H

#define VERSION 	"1.0"

extern char comdev[64];
extern char comarg[64];
extern int testflag;
extern int daemonflag;
extern int sockettype;
extern char socketaddr[64];

int option(int argc, char **argv);

#endif
