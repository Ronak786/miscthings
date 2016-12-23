#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "enclosure.h"

const char *lsscsi_cmnd = "lsscsi -g";
const char *sg_cmnd = "sg_ses -j";
static int get_boxname(char *name)
{
    FILE *fp;
    char vendor[LEN];
    char buf[200];

    if ((fp = popen(lsscsi_cmnd, "r")) == NULL)
            return -1; 
    while (fgets(buf, 200, fp) != NULL)
    {   
	sscanf(buf, "%*s %*s %s %*s %*s %*s %s", vendor, name);
//	printf("get vendor %s\n", vendor);
        if (strcmp(VENDOR, vendor) == 0) {
            pclose(fp);
            return 0;
        }   
    }   
    name[0] = '\0';
    pclose(fp);
    return -1; 
}
static int skip_until_slot(FILE *fp)
{
    char skipbuf[LEN];

    do {
        if (NULL == fgets(skipbuf, LEN, fp))
            return -1;
    } while ((strncmp("Slot", skipbuf, 4) != 0));
    return 0;
}

static int get_slot_info(const char *boxname, uint64_t *sas_addr)
{
    FILE *fp;
    char tmpbuf[LEN], *tmppos, namebuf[LEN];
    char headbuf[LEN], secondhead[LEN];
    int i, j;

    snprintf(namebuf, LEN,  "%s %s", sg_cmnd, boxname);
    if ((fp = popen(namebuf, "r")) == NULL) {
        fprintf(stderr, "error when exec %s\n", sg_cmnd);
        return -1; 
    }   
    for (i = 0; i < SLOTNUM; i++) {
        if (-1 == skip_until_slot(fp)) { //now when use fgets ,will get line just after "Slot xxxxxx"
            fprintf(stderr, "error in index %d and skip to slot\n");
            return -1; 
        }   

        for (j = 0; j < SKIPNUM; j++)
            fgets(tmpbuf, LEN-1, fp);
        fgets(tmpbuf, LEN, fp);
//	printf("get addr buf %s\n", tmpbuf);
        if (sscanf(tmpbuf, "%s %s %llx", headbuf, secondhead, &sas_addr[i]) != 3 ||  
            strcmp(headbuf, "SAS") || strcmp(secondhead, "address:")) {
            fprintf(stderr, "error index %d when parse sasaddr\n");
            return -1; 
        }   
//        printf("sas addr for slot %d: %llx\n", i, sas_addr[i]);
    }   
    return 0;
}


static int get_scsidev_info(struct sys_sas_dev sys_sas_dev)
{
	int i, fd;
	FILE *fp;
	DIR *dp;
	struct dirent *dirp;
	char scsinamebuf[LEN];
	char blocknamebuf[LEN];
	char devnamebuf[LEN];

	if ((dp = opendir(scsi_dev_host)) == NULL)
		return -1;
	//should check loop if will end
	for (i = 0; i < SASNUM; ) {
		if ((dirp = readdir(dp)) == NULL)
			break; // finished
		if (dirp->d_name[0] == '.')
			continue;  // skip . ..
		snprintf(scsinamebuf, LEN, "%s/%s", scsi_dev_host, dirp->d_name);
		if (get_value(dirp->d_name, "sas_address", &sys_sas_dev[i].sas_address) == -1)
			continue;  //this is not a real sas dev

		snprintf(blocknamebuf, LEN, "%s/%s", scsinamebuf, "block");
		if (get_dir_value(dirp->d_name, sys_sas_dev[i].devname) == -1)
			continue; // this should be the enclosure

		snprintf(devnamebuf, LEN, "%s/%s", blocknamebuf, sys_sas_dev[i].devname);
		if (get_value(dirp->d_name, "size", &sys_sas_dev[i].size) == -1)
			return -1;  //real sas dev,but read error

		if (get_value(scsinamebuf, "state", &sys_sas_dev[i].state) == -1)
			return -1;	//error can not read state
		i++; //success read one sas dev
	}
	closedir(dp);
	return i;
}


int main(void)
{
	char name[LEN];
	uint64_t sas_addr[SLOTNUM];
	int i;

	if (-1 == get_boxname(name)) {
		perror("why");
		return -1;
	}
	get_slot_info(name, sas_addr);
	for (i = 0; i < SLOTNUM; i++)
		printf("slot %d, sas_addr 0x%llx\n", i, sas_addr[i]);
	return 0;
}
 
