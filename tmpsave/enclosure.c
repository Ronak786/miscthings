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
static const char * sg_cmnd = "sg_ses -j ";
static const char * lsscsi_cmnd = "lsscsi -g";
static const char * blockdir = "/sys/block"

static int get_boxname(char *name);
static int get_slot_info(const char *boxname, uint64_t *sas_addr);
static int get_scsidev_info(struct sys_sas_dev sys_sas_dev);
static int get_scsidev_info(struct sys_sas_dev sys_sas_dev);
static int fill_enclosure(uint64_t *sg_sas_addr, struct sys_sas_dev *sys_sas_dev, 
			int sasnum, struct enclosure *enclosure);

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
	if (get_slot_info(boxname, sg_sas_addr) == -1) { //now we have slot i : sas_addr  pair
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
	char buf[200];

	if ((fp = popen(lsscsi_cmnd, "r")) == NULL)
			return -1;

	while (fgets(buf, 200, fp))
	{
		fscanf(fp, "%*s %*s %s %*s %*s %*s %s", vendor, name);
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

/*
 * get every slot's sas_address and stored into
 * array sas_addr
 */
static int get_slot_info(const char *boxname, uint64_t *sas_addr)
{
	FILE *fp;
	char tmpbuf[LEN], *tmppos, namebuf[LEN];
	char headbuf[LEN], secondhead[LEN];j

	snprintf(namebuf, LEN,  "%s %s", sg_cmnd, boxname);
	if ((fp = popen(sg_cmnd, "r")) == NULL) {
		fprintf(stderr, "error when exec %s\n", sg_cmnd);
		return -1;
	}
	for (i = 0; i < SLOTNUM; i++) {
		if (-1 == skip_until_slot(fp)) {//now when use fgets ,will get line just after "Slot xxxxxx"
			fprintf(stderr, "error in index %d and skip to slot\n");
			return -1;
		}

		for (j = 0; j < SKIPNUM; j++)
			fgets(tmpbuf, LEN-1, fp);
		fgets(tmpbuf, LEN, fp);

		if (sscanf(tmpbuf, "%s %s %lu", headbuf, secondhead, &sas_addr[i]) != 3 || 
			strcmp(headbuf, "SAS") || strcmp(secondhead, "address:")) {
			fprintf(stderr, "error index %d when parse sasaddr\n");
			return -1;
		}
		printf("sas addr for slot %d: %llx\n", i, sas_addr[i]);
	}
	return 0;
}

int get_value(char *filename, char *attr, void *arg)
{
	char path[LEN];
	FILE *fp;
	uint64_t block, sas_addr;
	char statebuf[LEN];
	enum dev_state state;

	snprintf(path, LEN, "%s/%s", filename, attr);

	if (!strcmp(attr, "sas_address")) {
		if ((fp = fopen(path, "r") == NULL) 
			return -1;
		if (fscanf(fp, "%llx", &sas_addr) == -1)
			return -1;
		fclose(fp);
		*(uint64_t*)arg = sas_addr;
	} else if (!strcmp(attr, "state")) {
		if ((fp = fopen(path, "r") == NULL) 
			return -1;
		if (fscanf(fp, "%s", statebuf) <= 0)
			return -1;
		if (!strcmp(statebuf, "running"))
			*(enum dev_state*)arg = ONLINE;
		else
			*(enum dev_state*)arg = OFFLINE;
		fclose(fp);
	} else if (!strcmp(attr, "size")) {
		if ((fp = fopen(path, "r") == NULL) 
			return -1;
		if (fscanf(fp, "%ld", &block) == -1)
			return -1;
		*(uint64_t*)arg = block;
		fclose(fp);
	} else
		return -1;
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

		
	
