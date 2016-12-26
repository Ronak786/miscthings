#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "enclosure.h"

static const char *sg_cmnd = "sg_ses -j";
static const char *scsi_dev_host = "/sys/bus/scsi/devices";

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
    struct stat mstat;
    char *searchdir[SLEN] = {"/usr/bin/", "/bin/", "/usr/local/bin/", "/usr/sbin/", "/usr/local/sbin/", 
				"/sbin/"};

    for ( i = 0; i < SLEN; i++) {
	snprintf(namebuf, LEN,  "%s%s", searchdir[i], "sg_ses");
	if (stat(namebuf, &mstat) != -1)
		break;
    }
    if (i == SLEN) {
	fprintf(stderr, "can not find command %s in all standard bin dir\n", namebuf);
	return -1;
    }

    snprintf(namebuf, LEN,  "%s%s %s", searchdir[i], sg_cmnd, boxname);
    if ((fp = popen(namebuf, "r")) == NULL) {
        fprintf(stderr, "error when exec %s\n", sg_cmnd);
        return -1; 
    }   
    for (i = 0; i < SLOTNUM; i++) {
        if (-1 == skip_until_slot(fp)) { //now when use fgets ,will get line just after "Slot xxxxxx"
            fprintf(stderr, "error in index %d and skip to slot\n");
	    pclose(fp);
            return -1; 
        }   

        for (j = 0; j < SKIPNUM; j++) 
            if (fgets(tmpbuf, LEN-1, fp) == NULL) { 
		pclose(fp);
		return -1;
	    }
	
        fgets(tmpbuf, LEN, fp);
        if (sscanf(tmpbuf, "%s %s %llx", headbuf, secondhead, &sas_addr[i]) != 3 ||  
            strcmp(headbuf, "SAS") || strcmp(secondhead, "address:")) {
            fprintf(stderr, "error index %d when parse sasaddr\n");
	    pclose(fp);
            return -1; 
        }   
    }   
    if (i != SLOTNUM) {
	fprintf(stderr, "error getting slotinfo\n");
	pclose(fp);
	return -1;
    }
    pclose(fp);
    return 0;
}

static int get_value(char *filename, char *attr, void *arg)
{
    char path[LEN];
    FILE *fp;
    uint64_t block, sas_addr;
    char statebuf[LEN];
    enum dev_state state;

    snprintf(path, LEN, "%s/%s", filename, attr);

    if (!strcmp(attr, "sas_address")) {
        if ((fp = fopen(path, "r")) == NULL)
            return -1;
        if (fscanf(fp, "%llx", &sas_addr) == -1)
            return -1;
        fclose(fp);
        *(uint64_t*)arg = sas_addr;
    } else if (!strcmp(attr, "state")) {
        if ((fp = fopen(path, "r")) == NULL)
            return -1;
        if (fscanf(fp, "%s", statebuf) <= 0)
            return -1;
        if (!strcmp(statebuf, "running"))
            *(enum dev_state*)arg = ONLINE;
        else
            *(enum dev_state*)arg = OFFLINE;
        fclose(fp);
    } else if (!strcmp(attr, "size")) {
        if ((fp = fopen(path, "r")) == NULL)
            return -1;
        if (fscanf(fp, "%ld", &block) == -1)
            return -1;
        *(uint64_t*)arg = block;
        fclose(fp);
    } else
        return -1;
    return 0;
}

static int get_dir_value(char *path, char *name)
{
    DIR *dp;
    struct dirent *dirp;

    if ((dp = opendir(path)) == NULL)
        return -1;
    while((dirp = readdir(dp))){
        if (dirp->d_name[0] == '.')
            continue;
        strncpy(name, dirp->d_name, LEN);
        closedir(dp);
        return 0;
    }
    return -1;
}

static int get_scsidev_info(struct sys_sas_dev *sys_sas_dev)
{
    int i, fd;
    FILE *fp;
    DIR *dp;
    struct dirent *dirp;
    char scsinamebuf[LEN];
    char blocknamebuf[LEN];
    char devnamebuf[LEN];

    if ((dp = opendir(scsi_dev_host)) == NULL) {
	fprintf(stderr, "wrong open dir %s\n", scsi_dev_host);
        return -1;
    }
    /*should check loop if will end*/
    for (i = 0; i < SASNUM; ) {
        if ((dirp = readdir(dp)) == NULL) {
            break; // finished all file search, so break
	}
        if (dirp->d_name[0] == '.')
            continue;  // skip . ..
        snprintf(scsinamebuf, LEN, "%s/%s", scsi_dev_host, dirp->d_name);
        if (get_value(scsinamebuf, "sas_address", &sys_sas_dev[i].sas_address) == -1)
            continue;  //this is not a real sas dev

        snprintf(blocknamebuf, LEN, "%s/%s", scsinamebuf, "block");
        if (get_dir_value(blocknamebuf, sys_sas_dev[i].devname) == -1)
            continue; // this should be the enclosure

        snprintf(devnamebuf, LEN, "%s/%s", blocknamebuf, sys_sas_dev[i].devname);
        if (get_value(devnamebuf, "size", &sys_sas_dev[i].block) == -1)
            return -1;  //real sas dev,but read error

        if (get_value(scsinamebuf, "state", &sys_sas_dev[i].state) == -1)
            return -1;  //error can not read state
        i++; //success read one sas dev
    }
    closedir(dp);
    return i;
}

static int get_boxname(char *name)
{
	FILE *fp;
	DIR *dp;
	struct dirent *dirp;
	char namebuf[LEN];
	char tmpbuf[LEN];

	if ((dp = opendir(scsi_dev_host)) == NULL)
		return -1;

	while ((dirp = readdir(dp))) {
		if (dirp->d_name[0] == '.')
			continue; //skip . and ..
		snprintf(namebuf, LEN, "%s/%s/vendor", scsi_dev_host, dirp->d_name);
		if ((fp = fopen(namebuf, "r")) == NULL)
			continue;  // dev/vendor not exist
		if (strncmp("LSI", fgets(tmpbuf, LEN, fp), 3) == 0) {
			snprintf(namebuf, LEN, "%s/%s/scsi_generic", scsi_dev_host, dirp->d_name);
			if (get_dir_value(namebuf, tmpbuf) == -1)
				return -1;
			snprintf(name, LEN, "/dev/%s", tmpbuf);
			return 0;
		}
	}
	return -1;
}
static int search_in_sys(uint64_t sg_sas_addr, struct sys_sas_dev *sys_sas_dev, int num)
{    
    int i;
    for (i = 0; i < num; i++) {        
	if (sg_sas_addr == sys_sas_dev[i].sas_address)            
		return i;   
    }      
     return -1; 
}

static int fill_enclosure(uint64_t *sg_sas_addr, struct sys_sas_dev *sys_sas_dev, 
			int sasnum, struct enclosure *enclosure)
{
    int slot = 0, i;

    for (slot = 0; slot < SLOTNUM; slot++) {
        i = search_in_sys(sg_sas_addr[slot], sys_sas_dev, sasnum);
        if (i == -1) {
            enclosure[slot].size = 0;
            enclosure[slot].state = EMPTY;
            enclosure[slot].devname[0] = '\0';
        } else {
            enclosure[slot].size = (sys_sas_dev[i].block << 9) / GB; 
            enclosure[slot].state = sys_sas_dev[i].state;
            strcpy(enclosure[slot].devname, sys_sas_dev[i].devname);
        }   
    }   
    return 0;
}

int get_enclosure_info(struct enclosure *enclosure)
{
	char name[LEN];
	uint64_t sas_addr[SLOTNUM];
	struct sys_sas_dev sys_sas_dev[SASNUM];
	int i, j;
	if (-1 == get_boxname(name)) {
		perror("why");
		return -1;
	}
	get_slot_info(name, sas_addr);
	i = get_scsidev_info(sys_sas_dev);
	fill_enclosure(sas_addr, sys_sas_dev, i, enclosure);
	return 0;
}
 
