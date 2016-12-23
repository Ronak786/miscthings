#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "enclosure.h"

//const char * sysroot = "/sys";
static const char * scsi_dev_host = "/sys/bus/scsi/devices";
static const char * sg_cmnd = "sg_ses -jff ";
static const char * lsscsi_cmnd = "lsscsi -g";

static int get_boxname(char *name);
static int get_slot_info(const char *boxname, uint64_t *sas_addr);
static int get_scsidev_info(struct sys_sas_dev sys_sas_dev);
static int get_scsidev_info(struct sys_sas_dev sys_sas_dev);
static int fill_enclosure(uint64_t *sg_sas_addr, struct sys_sas_dev *sys_sas_dev, int sasnum, struct enclosure *enclosure);

int main(void)
{
	FILE *fp;
	int i, err = 0;
	char boxname[LEN];
	uint64_t sg_sas_addr[SLOTNUM];
	struct sys_sas_dev sys_sas_dev[SAS_NUM]; //sasnum more than slotnum because enclosure has it or may others has it
	struct enclosure enclosure[SLOTNUM];
		
	if (get_boxname(boxname) == -1) {
		fprintf(stderr, "get boxname error %s\n", boxname); ///dev/sgxx
		exit(1);
	}
	if (get_slot_info(boxname, sg_sas_addr) == -1) //now we have slot i : sas_addr  pair
		fprintf(stderr, "get slot info using sg_ses error\n");
		exit(1);
	}
	i = get_scsidev_info(sys_sas_dev);
	if (i == -1) {
		fprintf(stderr, "error get sysfsinfo\n");
		exit(1);
	}
	if (fill_enclosure(sg_sas_addr, sys_sas_dev, i, enclosure) == -1) {
		fprintf(stderr, "error fill enclosure\n");
		exit(1);
	}

	for (i = 0; i < SLOTNUM; i++) 
		printf("slot %d, devname %s, state %d, size %lu\n", 
			i, en[i].devname, en[i].state, en[i].size);
	return 0;
}

/*
 * find /dev/sgxx as the enclosure for sg_ses
 * input: a buf for name storing
 * retval: -1 error
 *			0 success
 */
static int get_boxname(char *name)
{
	FILE *fp;
	char vendor[LEN];

	if ((fp = popen(lsscsi_cmnd, "r")) == NULL)
			return -1;

	while (fscanf(fp, "%s %*s %s %*s %*s %s", vendor, name) == 2)
	{
		if (strcmp(VENDOR, vendor) == 0) {
			pclose(fp);
			return 0;
		}
	}
	name[0] = '\0';
	pclose(fp);
	return -1;
}

/*
 * get every slot's sas_address and stored into
 * array sas_addr
 */
static int get_slot_info(const char *boxname, uint64_t *sas_addr)
{
	FILE *fp;
	char tmpbuf[LEN], *tmppos;

	if ((fp = popen(sg_cmnd, "r")) == NULL) {
		fprintf(stderr, "error when exec %s\n", sg_cmnd);
		exit(1);
	}
	for (i = 0; i < SLOTNUM; i++) {
		skip_until_slot(fp); //now when use fgets ,will get line just after "Slot xxxxxx"
		for (j = 0; j < SKIPNUM; j++)
			fgets(tmpbuf, LEN-1, fp);
		fgets(tmpbuf, LEN, fp);
		tmppos = check_and_skip(tmpbuf);
		if (tmppos == NULL)
			return -1;
		sas_addr[i] = strtol(tmppos, NULL, 16);
		printf("sas addr for slot %d: %llx\n", i, sas_addr[i]);
	}
	return 0;
}

static int get_scsidev_info(struct sys_sas_dev sys_sas_dev)
{
	int i, fd;
	FILE *fp;
	DIR *dp;
	struct dirent *dirp;
	char namebuf[LEN];

	fprintf(stderr, "opening %s\n", scsi_dev_host);
	if ((dp = opendir(scsi_dev_host)) == NULL)
		return -1;
	//should check loop if will end
	for (i = 0; i < SASNUM; ) {
		if ((dirp = readdir(dp)) == NULL)
			break; // over
		if (dirp->d_name[0] == '.')
			continue;  // skip . ..
		if (get_value(dirp->d_name, "sas_address", &sys_sas_dev[i].sas_address == -1)
			continue;  //this is not a real sas dev
		if (get_dir_value(dirp->d_name, "block", sys_sas_dev[i].devname) == -1
			continue; // this should be the enclosure
		if (get_value(dirp->d_name, "size", &sys_sas_dev[i].size) == -1)
			return -1;  //real sas dev,but read error
		if (get_value(dirp->d_name, "state", &sys_sas_dev[i].state) == -1)
			return -1;	//error can not read state
		i++; //success read one sas dev
	}
	closedir(dp);
	return i;
}

static int fill_enclosure(uint64_t *sg_sas_addr, struct sys_sas_dev *sys_sas_dev, int sasnum, struct enclosure *enclosure)
{
	int slot = 0, index;

	for (slot = 0; slot < SLOTNUM, slot++) {
		index = search_in_sys(sg_sas_addr[i], sys_sas_dev, sasnum);
		if (index == -1)
			return -1;
		enclosure[i].size = (sys_sas_dev[i].block << 9) / GB;
		enclosure[i].state = sys_sas_dev[i].state;
		strcpy(enclosure[i].devname, sys_sas_dev[i].devname);
		enclosure[i].slot = slot;
	}
	return 0;
}

		
	
