/*
 * Copyright (c) 2004-2016 Douglas Gilbert.
 * All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the BSD_LICENSE file.
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

#include <scsi/sg_lib.h>
#include <scsi/sg_cmds_basic.h>
#include <scsi/sg_cmds_extra.h>
#include <scsi/sg_pt.h>      /* needed for scsi_pt_win32_direct() */

#include "sg_unaligned.h"
#include "enclosure.h"

/*
 * This program issues SCSI SEND DIAGNOSTIC and RECEIVE DIAGNOSTIC RESULTS
 * commands tailored for SES (enclosure) devices.
 */
#define MX_ALLOC_LEN ((64 * 1024) - 4)  /* max allowable for big enclosures */
#define MX_ELEM_HDR 1024
#define MX_DATA_IN 2048
#define MX_JOIN_ROWS 260
#define NUM_ACTIVE_ET_AESP_ARR 32

#define TEMPERAT_OFF 20         /* 8 bits represents -19 C to +235 C */
                                /* value of 0 (would imply -20 C) reserved */

/* Send Diagnostic and Receive Diagnostic Results page codes */
#define DPC_SUPPORTED 0x0
#define DPC_CONFIGURATION 0x1
#define DPC_ENC_CONTROL 0x2
#define DPC_ENC_STATUS 0x2
#define DPC_HELP_TEXT 0x3
#define DPC_STRING 0x4
#define DPC_THRESHOLD 0x5
#define DPC_ARRAY_CONTROL 0x6   /* obsolete */
#define DPC_ARRAY_STATUS 0x6    /* obsolete */
#define DPC_ELEM_DESC 0x7
#define DPC_SHORT_ENC_STATUS 0x8
#define DPC_ENC_BUSY 0x9
#define DPC_ADD_ELEM_STATUS 0xa
#define DPC_SUBENC_HELP_TEXT 0xb
#define DPC_SUBENC_STRING 0xc
#define DPC_SUPPORTED_SES 0xd
#define DPC_DOWNLOAD_MICROCODE 0xe
#define DPC_SUBENC_NICKNAME 0xf

/* Element Type codes */
#define UNSPECIFIED_ETC 0x0
#define DEVICE_ETC 0x1
#define POWER_SUPPLY_ETC 0x2
#define COOLING_ETC 0x3
#define TEMPERATURE_ETC 0x4
#define DOOR_ETC 0x5    /* prior to ses3r05 was DOOR_LOCK_ETC */
#define AUD_ALARM_ETC 0x6
#define ENC_ELECTRONICS_ETC 0x7
#define SCC_CELECTR_ETC 0x8
#define NV_CACHE_ETC 0x9
#define INV_OP_REASON_ETC 0xa
#define UI_POWER_SUPPLY_ETC 0xb
#define DISPLAY_ETC 0xc
#define KEY_PAD_ETC 0xd
#define ENCLOSURE_ETC 0xe
#define SCSI_PORT_TRAN_ETC 0xf
#define LANGUAGE_ETC 0x10
#define COMM_PORT_ETC 0x11
#define VOLT_SENSOR_ETC 0x12
#define CURR_SENSOR_ETC 0x13
#define SCSI_TPORT_ETC 0x14
#define SCSI_IPORT_ETC 0x15
#define SIMPLE_SUBENC_ETC 0x16
#define ARRAY_DEV_ETC 0x17
#define SAS_EXPANDER_ETC 0x18
#define SAS_CONNECTOR_ETC 0x19
#define LAST_ETC SAS_CONNECTOR_ETC      /* adjust as necessary */

#define NUM_ETC (LAST_ETC + 1)


struct element_type_t {
    int elem_type_code;
    const char * abbrev;
    const char * desc;
};

struct opts_t {
    int byte1;
    int byte1_given;
    int do_control;
    int do_data;
    int dev_slot_num;
    int enumerate;
    int eiioe_auto;
    int eiioe_force;
    int do_filter;
    int do_help;
    int do_hex;
    int ind_given;
    int ind_th;         /* type header index */
    int ind_indiv;      /* individual element index; -1 for overall */
    int ind_et_inst;    /* ETs can have multiple type header instances */
    int inner_hex;
    int do_join;
    int do_list;
    int mask_ign;       /* element read-mask-modify-write actions */
    int maxlen;
    int seid;
    int seid_given;
    int page_code;
    int page_code_given;
    int do_raw;
    int o_readonly;
    int do_status;
    int verbose;
    int do_version;
    int warn;
    int num_cgs;
    int arr_len;
    unsigned char sas_addr[8];
    unsigned char data_arr[MX_DATA_IN + 16];
    const char * clear_str;
    const char * desc_name;
    const char * get_str;
    const char * set_str;
    const char * dev_name;
    const char * index_str;
    const char * nickname_str;
    const struct element_type_t * ind_etp;
};

struct diag_page_code {
    int page_code;
    const char * desc;
};

/* The Configuration diagnostic page contains one or more of these. The
 * elements of the Enclosure Control/Status and Threshold In/ Out page follow
 * this format. The additional element status page is closely related to
 * this format (with some element types and all overall elements excluded). */
struct type_desc_hdr_t {
    unsigned char etype;        /* element type code (0: unspecified) */
    unsigned char num_elements; /* number of possible elements, excluding
                                 * overall element */
    unsigned char se_id;        /* subenclosure id (0 for primary enclosure) */
    unsigned char txt_len;      /* type descriptor text length; (unused) */
};

/* A SQL-like join of the Enclosure Status, Threshold In and Additional
 * Element Status pages based of the format indicated in the Configuration
 * page. */
struct join_row_t {
    int el_ind_th;              /* type header index (origin 0) */
    int el_ind_indiv;           /* individual element index, -1 for overall
                                 * instance, otherwise origin 0 */
    unsigned char etype;        /* element type */
    unsigned char se_id;        /* subenclosure id (0 for primary enclosure) */
    int ei_asc;                 /* element index used by Additional Element
                                 * Status page, -1 for not applicable */
    int ei_asc2;                /* some vendors get ei_asc wrong, this is
                                 * their broken version */
    /* following point into Element Descriptor, Enclosure Status, Threshold
     * In and Additional element status diagnostic pages. enc_statp only
     * NULL past last, other pointers can be NULL . */
    unsigned char * elem_descp;
    unsigned char * enc_statp;  /* NULL indicates past last */
    unsigned char * thresh_inp;
    unsigned char * add_elem_statp;
    int dev_slot_num;           /* if not available, set to -1 */
    unsigned char sas_addr[8];  /* if not available, set to 0 */
};

/* Structure for holding (sub-)enclosure information found in the
 * Configuration diagnostic page. */
struct enclosure_info {
    int have_info;
    int rel_esp_id;     /* relative enclosure services process id (origin 1) */
    int num_esp;        /* number of enclosure services processes */
    unsigned char enc_log_id[8];        /* 8 byte NAA */
    unsigned char enc_vendor_id[8];     /* may differ from INQUIRY response */
    unsigned char product_id[16];       /* may differ from INQUIRY response */
    unsigned char product_rev_level[4]; /* may differ from INQUIRY response */
};

static struct type_desc_hdr_t type_desc_hdr_arr[MX_ELEM_HDR];

static struct join_row_t join_arr[MX_JOIN_ROWS];
static struct join_row_t * join_arr_lastp = join_arr + MX_JOIN_ROWS - 1;

static unsigned char enc_stat_rsp[MX_ALLOC_LEN];
static unsigned char elem_desc_rsp[MX_ALLOC_LEN];
static unsigned char add_elem_rsp[MX_ALLOC_LEN];
static unsigned char threshold_rsp[MX_ALLOC_LEN];

static int enc_stat_rsp_len;
static int elem_desc_rsp_len;
static int add_elem_rsp_len;
static int threshold_rsp_len;

/* Diagnostic page names, for status (or in) pages */
static struct diag_page_code in_dpc_arr[] = {
    {DPC_SUPPORTED, "Supported Diagnostic Pages"},  /* 0 */
    {DPC_CONFIGURATION, "Configuration (SES)"},
    {DPC_ENC_STATUS, "Enclosure Status (SES)"},
    {DPC_HELP_TEXT, "Help Text (SES)"},
    {DPC_STRING, "String In (SES)"},
    {DPC_THRESHOLD, "Threshold In (SES)"},
    {DPC_ARRAY_STATUS, "Array Status (SES, obsolete)"},
    {DPC_ELEM_DESC, "Element Descriptor (SES)"},
    {DPC_SHORT_ENC_STATUS, "Short Enclosure Status (SES)"},  /* 8 */
    {DPC_ENC_BUSY, "Enclosure Busy (SES-2)"},
    {DPC_ADD_ELEM_STATUS, "Additional Element Status (SES-2)"},
    {DPC_SUBENC_HELP_TEXT, "Subenclosure Help Text (SES-2)"},
    {DPC_SUBENC_STRING, "Subenclosure String In (SES-2)"},
    {DPC_SUPPORTED_SES, "Supported SES Diagnostic Pages (SES-2)"},
    {DPC_DOWNLOAD_MICROCODE, "Download Microcode (SES-2)"},
    {DPC_SUBENC_NICKNAME, "Subenclosure Nickname (SES-2)"},
    {0x3f, "Protocol Specific (SAS transport)"},
    {0x40, "Translate Address (SBC)"},
    {0x41, "Device Status (SBC)"},
    {0x42, "Rebuild Assist Input (SBC)"},
    {-1, NULL},
};

/* Boolean array of element types of interest to the Additional Element
 * Status page. Indexed by element type (0 <= et <= 32). */
static int active_et_aesp_arr[NUM_ACTIVE_ET_AESP_ARR] = {
    0, 1 /* dev */, 0, 0,  0, 0, 0, 1 /* esce */,
    0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  1 /* starg */, 1 /* sinit */, 0, 1 /* arr */,
    1 /* sas exp */, 0, 0, 0,  0, 0, 0, 0,
};

/* Command line long option names with corresponding short letter. */
static struct option long_options[] = {
    {"join", no_argument, 0, 'j'},
    {0, 0, 0, 0},
};

static int
cl_process(struct opts_t *op, char *boxname)
{
    int c, j, ret, ff;
    const char * data_arg = NULL;
    uint64_t saddr;
    const char * cp;

    op->dev_slot_num = -1;
    op->do_join++;
    op->dev_name = boxname;
    if (op->maxlen <= 0)
        op->maxlen = MX_ALLOC_LEN;

    if (0 == op->do_status)
        op->do_status = 1;  /* default to receiving status pages */

    if (NULL == op->dev_name) {
        printf("missing DEVICE name!\n");
        goto err_help;
    }
    return 0;

err_help:
    printf("  For more information use '--help'\n");
    return SG_LIB_SYNTAX_ERROR;
}

/* Fetch diagnostic page name (status or in). Returns NULL if not found. */
static const char *
find_in_diag_page_desc(int page_num)
{
    const struct diag_page_code * pcdp;

    for (pcdp = in_dpc_arr; pcdp->desc; ++pcdp) {
        if (page_num == pcdp->page_code)
            return pcdp->desc;
        else if (page_num < pcdp->page_code)
            return NULL;
    }
    return NULL;
}


/* Returns 1 if el_type (element type) is of interest to the Additional
 * Element Status page. Otherwise return 0. */
static int
active_et_aesp(int el_type)
{
    if ((el_type >= 0) && (el_type < NUM_ACTIVE_ET_AESP_ARR))
        return active_et_aesp_arr[el_type];
    else
        return 0;
}

/* Return of 0 -> success, SG_LIB_CAT_* positive values or -1 -> other
 * failures */
static int
do_rec_diag(int sg_fd, int page_code, unsigned char * rsp_buff,
            int rsp_buff_size, const struct opts_t * op, int * rsp_lenp)
{
    int rsp_len, res;
    const char * cp;
    char b[80];

    memset(rsp_buff, 0, rsp_buff_size);
    if (rsp_lenp)
        *rsp_lenp = 0;
    cp = find_in_diag_page_desc(page_code);
    if (op->verbose > 1) {
        if (cp)
            printf("    Receive diagnostic results cmd for %s page\n", cp);
        else
            printf("    Receive diagnostic results cmd for page 0x%x\n",
                    page_code);
    }
    res = sg_ll_receive_diag(sg_fd, 1 /* pcv */, page_code, rsp_buff,
                             rsp_buff_size, 1, op->verbose);
    if (0 == res) {
        rsp_len = sg_get_unaligned_be16(rsp_buff + 2) + 4;
        if (rsp_len > rsp_buff_size) {
            if (rsp_buff_size > 8) /* tried to get more than header */
                printf("<<< warning response buffer too small [%d but need "
                        "%d]>>>\n", rsp_buff_size, rsp_len);
            rsp_len = rsp_buff_size;
        }
        if (rsp_lenp)
            *rsp_lenp = rsp_len;
        if (page_code != rsp_buff[0]) {
            if ((0x9 == rsp_buff[0]) && (1 & rsp_buff[1])) {
                printf("Enclosure busy, try again later\n");
            } else if (0x8 == rsp_buff[0]) {
                printf("Enclosure only supports Short Enclosure Status: "
                        "0x%x\n", rsp_buff[1]);
            } else {
                printf("Invalid response, wanted page code: 0x%x but got "
                        "0x%x\n", page_code, rsp_buff[0]);
            }
            return -2;
        }
        return 0;
    } else if (op->verbose) {
        if (cp)
            printf("Attempt to fetch %s diagnostic page failed\n", cp);
        else
            printf("Attempt to fetch status diagnostic page [0x%x] failed\n",
                    page_code);
        sg_get_category_sense_str(res, sizeof(b), b, op->verbose);
        printf("    %s\n", b);
    }
    return res;
}

/* DPC_CONFIGURATION
 * Returns total number of type descriptor headers written to 'tdhp' or -1
 * if there is a problem */
static int
populate_type_desc_hdr_arr(int fd, struct type_desc_hdr_t * tdhp,
                           uint32_t * generationp,
                           struct enclosure_info * primary_ip,
                           struct opts_t * op)
{
    int resp_len, k, el, num_subs, sum_type_dheaders, res, n;
    int ret = 0;
    uint32_t gen_code;
    unsigned char * resp;
    const unsigned char * ucp;
    const unsigned char * last_ucp;

    resp = (unsigned char *)calloc(op->maxlen, 1);
    if (NULL == resp) {
        printf("%s: unable to allocate %d bytes on heap\n", __func__,
                op->maxlen);
        ret = -1;
        goto the_end;
    }
    res = do_rec_diag(fd, DPC_CONFIGURATION, resp, op->maxlen, op, &resp_len);
    if (res) {
        printf("%s: couldn't read config page, res=%d\n", __func__, res);
        ret = -1;
        goto the_end;
    }
    if (resp_len < 4) {
        ret = -1;
        goto the_end;
    }
    num_subs = resp[1] + 1;
    sum_type_dheaders = 0;
    last_ucp = resp + resp_len - 1;
    gen_code = sg_get_unaligned_be32(resp + 4);
    if (generationp)
        *generationp = gen_code;
    ucp = resp + 8;
    for (k = 0; k < num_subs; ++k, ucp += el) {
        if ((ucp + 3) > last_ucp)
            goto p_truncated;
        el = ucp[3] + 4;
        sum_type_dheaders += ucp[2];
        if (el < 40) {
            printf("%s: short enc descriptor len=%d ??\n", __func__, el);
            continue;
        }
        if ((0 == k) && primary_ip) {
            ++primary_ip->have_info;
            primary_ip->rel_esp_id = (ucp[0] & 0x70) >> 4;
            primary_ip->num_esp = (ucp[0] & 0x7);
            memcpy(primary_ip->enc_log_id, ucp + 4, 8);
            memcpy(primary_ip->enc_vendor_id, ucp + 12, 8);
            memcpy(primary_ip->product_id, ucp + 20, 16);
            memcpy(primary_ip->product_rev_level, ucp + 36, 4);
        }
    }
    for (k = 0; k < sum_type_dheaders; ++k, ucp += 4) {
        if ((ucp + 3) > last_ucp)
            goto p_truncated;
        if (k >= MX_ELEM_HDR) {
            printf("%s: too many elements\n", __func__);
            ret = -1;
            goto the_end;
        }
        tdhp[k].etype = ucp[0];
        tdhp[k].num_elements = ucp[1];
        tdhp[k].se_id = ucp[2];
        tdhp[k].txt_len = ucp[3];
    }
    if (op->ind_given && op->ind_etp) {
        n = op->ind_et_inst;
        for (k = 0; k < sum_type_dheaders; ++k) {
            if (op->ind_etp->elem_type_code == tdhp[k].etype) {
                if (0 == n)
                    break;
                else
                    --n;
            }
        }
        if (k < sum_type_dheaders)
            op->ind_th = k;
        else {
            if (op->ind_et_inst)
                printf("%s: unable to find element type '%s%d'\n", __func__,
                        op->ind_etp->abbrev, op->ind_et_inst);
            else
                printf("%s: unable to find element type '%s'\n", __func__,
                        op->ind_etp->abbrev);
            ret = -1;
            goto the_end;
        }
    }
    ret = sum_type_dheaders;
    goto the_end;

p_truncated:
    printf("%s: config too short\n", __func__);
    ret = -1;

the_end:
    if (resp)
        free(resp);
    return ret;
}

static const char * elem_status_code_desc[] = {
    "Unsupported", "OK", "Critical", "Noncritical",
    "Unrecoverable", "Not installed", "Unknown", "Not available",
    "No access allowed", "reserved [9]", "reserved [10]", "reserved [11]",
    "reserved [12]", "reserved [13]", "reserved [14]", "reserved [15]",
};

static const char * actual_speed_desc[] = {
    "stopped", "at lowest speed", "at second lowest speed",
    "at third lowest speed", "at intermediate speed",
    "at third highest speed", "at second highest speed", "at highest speed"
};

static const char * nv_cache_unit[] = {
    "Bytes", "KiB", "MiB", "GiB"
};

static const char * invop_type_desc[] = {
    "SEND DIAGNOSTIC page code error", "SEND DIAGNOSTIC page format error",
    "Reserved", "Vendor specific error"
};

static int
saddr_non_zero(const unsigned char * ucp)
{
    int k;

    for (k = 0; k < 8; ++k) {
        if (ucp[k])
            return 1;
    }
    return 0;
}

static const char * sas_device_type[] = {
    "no SAS device attached",   /* but might be SATA device */
    "end device",
    "expander device",  /* in SAS-1.1 this was a "edge expander device */
    "expander device (fanout, SAS-1.1)",  /* marked obsolete in SAS-2 */
    "reserved [4]", "reserved [5]", "reserved [6]", "reserved [7]"
};

static int
additional_elem_helper(const char * pad, const unsigned char * ucp, int len,
                       int elem_type, const struct opts_t * op, uint64_t *sas_addr, int num)
{
    int ports, phys, j, m, desc_type, eip_offset, print_sas_addr, saddr_nz;
    int nofilter = ! op->do_filter, slot;
    uint16_t pcie_vid;
    int pcie_pt, psn_valid, bdf_valid, cid_valid;
    const unsigned char * per_ucp;
    char b[64];
    char tmpbuf[20], *bufptr = tmpbuf;

    memset(tmpbuf, 0, 20);
    if (op->inner_hex) {
        for (j = 0; j < len; ++j) {
            if (0 == (j % 16))
                printf("%s%s", ((0 == j) ? "" : "\n"), pad);
            printf("%02x ", ucp[j]);
        }
        printf("\n");
        return;
    }
    eip_offset = (0x10 & ucp[0]) ? 2 : 0;
    switch (0xf & ucp[0]) {
    case TPROTO_SAS:
        if (len < (4 + eip_offset))
            break;
        desc_type = (ucp[3 + eip_offset] >> 6) & 0x3;
        if (0 == desc_type) {
            phys = ucp[2 + eip_offset];
            if (eip_offset)
                slot = ucp[5 + eip_offset];
            per_ucp = ucp + 4 + eip_offset + eip_offset;
            for (j = 0; j < phys; ++j, per_ucp += 28) {
                saddr_nz = saddr_non_zero(per_ucp + 12);
	   	if (saddr_nz) {
	   	    for (m = 0; m < 8; ++m)
			bufptr += snprintf(bufptr, 3, "%02x", per_ucp[12 + m]);
	   	} else
			tmpbuf[0] = '0'; //slot is emtpy
            }
	    if ( (slot < num) && (sscanf(tmpbuf, "%llx", &sas_addr[slot]) != 1))
		return -1;
        } 
        break;
    default:
        printf("%sTransport protocol: %s not decoded\n", pad,
               sg_get_trans_proto_str((0xf & ucp[0]), sizeof(b), b));
        break;
    }
}


/* An array of Download microcode status field values and descriptions */
static struct diag_page_code mc_status_arr[] = {
    {0x0, "No download microcode operation in progress"},
    {0x1, "Download in progress, awaiting more"},
    {0x2, "Download complete, updating storage"},
    {0x3, "Updating storage with deferred microcode"},
    {0x10, "Complete, no error, starting now"},
    {0x11, "Complete, no error, start after hard reset or power cycle"},
    {0x12, "Complete, no error, start after power cycle"},
    {0x13, "Complete, no error, start after activate_mc, hard reset or "
           "power cycle"},
    {0x80, "Error, discarded, see additional status"},
    {0x81, "Error, discarded, image error"},
    {0x82, "Timeout, discarded"},
    {0x83, "Internal error, need new microcode before reset"},
    {0x84, "Internal error, need new microcode, reset safe"},
    {0x85, "Unexpected activate_mc received"},
    {0x1000, NULL},
};

static void
devslotnum_and_sasaddr(struct join_row_t * jrp, unsigned char * ae_ucp)
{
    int m;

    if ((0 == jrp) || (0 == ae_ucp) || (0 == (0x10 & ae_ucp[0])))
        return; /* sanity and expect EIP=1 */
    switch (0xf & ae_ucp[0]) {
    case TPROTO_FCP:
        jrp->dev_slot_num = ae_ucp[7];
        break;
    case TPROTO_SAS:
        if (0 == (0xc0 & ae_ucp[5])) {
            /* only for device slot and array device slot elements */
            jrp->dev_slot_num = ae_ucp[7];
            if (ae_ucp[4] > 0) {        /* number of phys */
                /* Use the first phy's "SAS ADDRESS" field */
                for (m = 0; m < 8; ++m)
                    jrp->sas_addr[m] = ae_ucp[(4 + 4 + 12) + m];
            }
        }
        break;
    case TPROTO_PCIE:
        jrp->dev_slot_num = ae_ucp[7];
        break;
    default:
        ;
    }
}

/* Fetch Configuration, Enclosure Status, Element Descriptor, Additional
 * Element Status and optionally Threshold In pages, place in static arrays.
 * Collate (join) overall and individual elements into the static join_arr[].
 * Returns 0 for success, any other return value is an error. */
static int
join_work(int sg_fd, struct opts_t * op, int display, uint64_t * sas_addr, int num)
{
    int k, j, res, num_t_hdrs, elem_ind, ei, desc_len, dn_len;
    int et4aes, broken_ei, ei2, got1, jr_max_ind, mlen;
    uint32_t ref_gen_code, gen_code;
    struct join_row_t * jrp;
    struct join_row_t * jr2p;
    unsigned char * es_ucp;
    unsigned char * ed_ucp;
    unsigned char * ae_ucp;
    unsigned char * t_ucp;
    const unsigned char * ae_last_ucp;
    const char * cp;
    const char * enc_state_changed = "  <<state of enclosure changed, "
                                     "please try again>>\n";
    const struct type_desc_hdr_t * tdhp;
    struct enclosure_info primary_info;
    char b[64];

    memset(&primary_info, 0, sizeof(primary_info));
    num_t_hdrs = populate_type_desc_hdr_arr(sg_fd, type_desc_hdr_arr,
                                            &ref_gen_code, &primary_info,
                                            op);
    if (num_t_hdrs < 0)
        return num_t_hdrs;
    mlen = sizeof(enc_stat_rsp);
    if (mlen > op->maxlen)
        mlen = op->maxlen;
    res = do_rec_diag(sg_fd, DPC_ENC_STATUS, enc_stat_rsp, mlen, op,
                      &enc_stat_rsp_len);
    if (res)
        return res;
    if (enc_stat_rsp_len < 8) {
        printf("Enclosure Status response too short\n");
        return -1;
    }
    gen_code = sg_get_unaligned_be32(enc_stat_rsp + 4);
    if (ref_gen_code != gen_code) {
        printf("%s", enc_state_changed);
        return -1;
    }
    es_ucp = enc_stat_rsp + 8;
    /* es_last_ucp = enc_stat_rsp + enc_stat_rsp_len - 1; */

    mlen = sizeof(elem_desc_rsp);
    if (mlen > op->maxlen)
        mlen = op->maxlen;
    res = do_rec_diag(sg_fd, DPC_ELEM_DESC, elem_desc_rsp, mlen, op,
                      &elem_desc_rsp_len);
    if (0 == res) {
        if (elem_desc_rsp_len < 8) {
            printf("Element Descriptor response too short\n");
            return -1;
        }
        gen_code = sg_get_unaligned_be32(elem_desc_rsp + 4);
        if (ref_gen_code != gen_code) {
            printf("%s", enc_state_changed);
            return -1;
        }
        ed_ucp = elem_desc_rsp + 8;
        /* ed_last_ucp = elem_desc_rsp + elem_desc_rsp_len - 1; */
    } else {
        elem_desc_rsp_len = 0;
        ed_ucp = NULL;
        res = 0;
        if (op->verbose)
            printf("  Element Descriptor page not available\n");
    }

    if (display || (DPC_ADD_ELEM_STATUS == op->page_code) ||
        (op->dev_slot_num >= 0) || saddr_non_zero(op->sas_addr)) {
        mlen = sizeof(add_elem_rsp);
        if (mlen > op->maxlen)
            mlen = op->maxlen;
        res = do_rec_diag(sg_fd, DPC_ADD_ELEM_STATUS, add_elem_rsp, mlen, op,
                          &add_elem_rsp_len);
        if (0 == res) {
            if (add_elem_rsp_len < 8) {
                printf("Additional Element Status response too short\n");
                return -1;
            }
            gen_code = sg_get_unaligned_be32(add_elem_rsp + 4);
            if (ref_gen_code != gen_code) {
                printf("%s", enc_state_changed);
                return -1;
            }
            ae_ucp = add_elem_rsp + 8;
            ae_last_ucp = add_elem_rsp + add_elem_rsp_len - 1;
            if (op->eiioe_auto && (add_elem_rsp_len > 11)) {
                /* heuristic: if first element index in this page is 1
                 * then act as if the EIIOE bit is set. */
                if ((ae_ucp[0] & 0x10) && (1 == ae_ucp[3]))
                    op->eiioe_force = 1;
            }
        } else {
            add_elem_rsp_len = 0;
            ae_ucp = NULL;
            ae_last_ucp = NULL;
            res = 0;
            if (op->verbose)
                printf("  Additional Element Status page not available\n");
        }
    } else {
        ae_ucp = NULL;
        ae_last_ucp = NULL;
    }

    if ((op->do_join > 1) ||
        ((0 == display) && (DPC_THRESHOLD == op->page_code))) {
        mlen = sizeof(threshold_rsp);
        if (mlen > op->maxlen)
            mlen = op->maxlen;
        res = do_rec_diag(sg_fd, DPC_THRESHOLD, threshold_rsp, mlen, op,
                          &threshold_rsp_len);
        if (0 == res) {
            if (threshold_rsp_len < 8) {
                printf("Threshold In response too short\n");
                return -1;
            }
            gen_code = sg_get_unaligned_be32(threshold_rsp + 4);
            if (ref_gen_code != gen_code) {
                printf("%s", enc_state_changed);
                return -1;
            }
            t_ucp = threshold_rsp + 8;
            /* t_last_ucp = threshold_rsp + threshold_rsp_len - 1; */
        } else {
            threshold_rsp_len = 0;
            t_ucp = NULL;
            res = 0;
            if (op->verbose)
                printf("  Threshold In page not available\n");
        }
    } else {
        threshold_rsp_len = 0;
        t_ucp = NULL;
    }

    jrp = join_arr;
    tdhp = type_desc_hdr_arr;
    jr_max_ind = 0;
    for (k = 0, ei = 0, ei2 = 0; k < num_t_hdrs; ++k, ++tdhp) {
        jrp->el_ind_th = k;
        jrp->el_ind_indiv = -1;
        jrp->etype = tdhp->etype;
        jrp->ei_asc = -1;
        et4aes = active_et_aesp(tdhp->etype);
        jrp->ei_asc2 = -1;
        jrp->se_id = tdhp->se_id;
        /* check es_ucp < es_last_ucp still in range */
        jrp->enc_statp = es_ucp;
        es_ucp += 4;
        jrp->elem_descp = ed_ucp;
        if (ed_ucp)
            ed_ucp += sg_get_unaligned_be16(ed_ucp + 2) + 4;
        jrp->add_elem_statp = NULL;
        jrp->thresh_inp = t_ucp;
        jrp->dev_slot_num = -1;
        /* assume sas_addr[8] zeroed since it's static file scope */
        if (t_ucp)
            t_ucp += 4;
        ++jrp;
        for (j = 0, elem_ind = 0; j < tdhp->num_elements;
             ++j, ++jrp, ++elem_ind) {
            if (jrp >= join_arr_lastp)
                break;
            jrp->el_ind_th = k;
            jrp->el_ind_indiv = elem_ind;
            jrp->ei_asc = ei++;
            if (et4aes)
                jrp->ei_asc2 = ei2++;
            else
                jrp->ei_asc2 = -1;
            jrp->etype = tdhp->etype;
            jrp->se_id = tdhp->se_id;
            jrp->enc_statp = es_ucp;
            es_ucp += 4;
            jrp->elem_descp = ed_ucp;
            if (ed_ucp)
                ed_ucp += sg_get_unaligned_be16(ed_ucp + 2) + 4;
            jrp->thresh_inp = t_ucp;
            jrp->dev_slot_num = -1;
            /* assume sas_addr[8] zeroed since it's static file scope */
            if (t_ucp)
                t_ucp += 4;
            jrp->add_elem_statp = NULL;
            ++jr_max_ind;
        }
        if (jrp >= join_arr_lastp) {
            ++k;
            break;      /* leave last row all zeros */
        }
    }

    broken_ei = 0;
    if (ae_ucp) {
        int eip, eiioe;
        int aes_i = 0;
        int get_out = 0;

        jrp = join_arr;
        tdhp = type_desc_hdr_arr;
        for (k = 0; k < num_t_hdrs; ++k, ++tdhp) {
            if (active_et_aesp(tdhp->etype)) {
                /* only consider element types that AES element are permiited
                 * to refer to, then loop over those number of elements */
                for (j = 0; j < tdhp->num_elements; ++j) {
                    if ((ae_ucp + 1) > ae_last_ucp) {
                        get_out = 1;
                        if (op->verbose || op->warn)
                            printf("warning: %s: off end of ae page\n",
                                    __func__);
                        break;
                    }
                    eip = !!(ae_ucp[0] & 0x10); /* element index present */
                    if (eip)
                        eiioe = op->eiioe_force ? 1 : (ae_ucp[2] & 1);
                    else
                        eiioe = 0;
                    if (eip && eiioe) {
                        ei = ae_ucp[3];
                        jr2p = join_arr + ei;
                        if ((ei >= jr_max_ind) || (NULL == jr2p->enc_statp)) {
                            get_out = 1;
                            printf("%s: oi=%d, ei=%d [max_ind=%d], eiioe=1 "
                                    "not in join_arr\n", __func__, k, ei,
                                    jr_max_ind);
                            break;
                        }
                        devslotnum_and_sasaddr(jr2p, ae_ucp);
                        if (jr2p->add_elem_statp) {
                            if (op->warn || op->verbose)
                                printf("warning: aes slot busy [oi=%d, "
                                        "ei=%d, aes_i=%d]\n", k, ei, aes_i);
                        } else
                            jr2p->add_elem_statp = ae_ucp;
                    } else if (eip) {     /* and EIIOE=0 */
                        ei = ae_ucp[3];
try_again:
                        for (jr2p = join_arr; jr2p->enc_statp; ++jr2p) {
                            if (broken_ei) {
                                if (ei == jr2p->ei_asc2)
                                    break;
                            } else {
                                if (ei == jr2p->ei_asc)
                                    break;
                            }
                        }
                        if (NULL == jr2p->enc_statp) {
                            get_out = 1;
                            printf("warning: %s: oi=%d, ei=%d (broken_ei=%d) "
                                    "not in join_arr\n", __func__, k, ei,
                                    broken_ei);
                            break;
                        }
                        if (! active_et_aesp(jr2p->etype)) {
                            /* broken_ei must be 0 for that to be false */
                            ++broken_ei;
                            goto try_again;
                        }
                        devslotnum_and_sasaddr(jr2p, ae_ucp);
                        if (jr2p->add_elem_statp) {
                            if (op->warn || op->verbose)
                                printf("warning: aes slot busy [oi=%d, "
                                        "ei=%d, aes_i=%d]\n", k, ei, aes_i);
                        } else
                            jr2p->add_elem_statp = ae_ucp;
                    } else {    /* EIP=0 */
                        while (jrp->enc_statp && ((-1 == jrp->el_ind_indiv) ||
                                                  jrp->add_elem_statp))
                            ++jrp;
                        if (NULL == jrp->enc_statp) {
                            get_out = 1;
                            printf("warning: %s: join_arr has no space for "
                                    "ae\n", __func__);
                            break;
                        }
                        jrp->add_elem_statp = ae_ucp;
                        ++jrp;
                    }
                    ae_ucp += ae_ucp[1] + 2;
                    ++aes_i;
                }
            } else {    /* element type not relevant to ae status */
                /* step over overall and individual elements */
                for (j = 0; j <= tdhp->num_elements; ++j, ++jrp) {
                    if (NULL == jrp->enc_statp) {
                        get_out = 1;
                        printf("warning: %s: join_arr has no space\n",
                                __func__);
                        break;
                    }
                }
            }
            if (get_out)
                break;
        }
    }
    /* Display contents of join_arr */
    dn_len = op->desc_name ? (int)strlen(op->desc_name) : 0;
    for (k = 0, jrp = join_arr, got1 = 0;
         ((k < MX_JOIN_ROWS) && jrp->enc_statp); ++k, ++jrp) {
        if (op->ind_given) {
            if (op->ind_th != jrp->el_ind_th)
                continue;
            if (op->ind_indiv != jrp->el_ind_indiv)
                continue;
        }
        ed_ucp = jrp->elem_descp;
        if (op->desc_name) {
            if (NULL == ed_ucp)
                continue;
            desc_len = sg_get_unaligned_be16(ed_ucp + 2);
            /* some element descriptor strings have trailing NULLs and
             * count them in their length; adjust */
            while (desc_len && ('\0' == ed_ucp[4 + desc_len - 1]))
                --desc_len;
            if (desc_len != dn_len)
                continue;
            if (0 != strncmp(op->desc_name, (const char *)(ed_ucp + 4),
                             desc_len))
                continue;
        } else if (op->dev_slot_num >= 0) {
            if (op->dev_slot_num != jrp->dev_slot_num)
                continue;
        } else if (saddr_non_zero(op->sas_addr)) {
            for (j = 0; j < 8; ++j) {
                if (op->sas_addr[j] != jrp->sas_addr[j])
                    break;
            }
            if (j < 8)
                continue;
        }
        ++got1;
        if ((op->do_filter > 1) && (1 != (0xf & jrp->enc_statp[0])))
            continue;   /* when '-ff' and status!=OK, skip */
        if (jrp->add_elem_statp) {
            ae_ucp = jrp->add_elem_statp;
            desc_len = ae_ucp[1] + 2;
            if (additional_elem_helper("    ",  ae_ucp, desc_len, jrp->etype, op, sas_addr, num) == -1)
		return -1;
        }
    }
    if (0 == got1) {
        if (op->ind_given)
            printf("      >>> no match on --index=%d,%d\n", op->ind_th,
                   op->ind_indiv);
        else if (op->desc_name)
            printf("      >>> no match on --descriptor=%s\n", op->desc_name);
        else if (op->dev_slot_num >= 0)
            printf("      >>> no match on --dev-slot-name=%d\n",
                   op->dev_slot_num);
        else if (saddr_non_zero(op->sas_addr)) {
            printf("      >>> no match on --sas-addr=0x");
            for (j = 0; j < 8; ++j)
                printf("%02x", op->sas_addr[j]);
            printf("\n");
        }
    }
    return res;
}



int
get_slot_info(char *boxname, uint64_t *sas_addr, int num)
{
    int sg_fd, res;
    char buff[128];
    char b[80];
    int pd_type = 0;
    int have_cgs = 0;
    int ret = 0;
    struct sg_simple_inquiry_resp inq_resp;
    const char * cp;
    struct opts_t opts;
    struct opts_t * op;

    op = &opts;
    memset(op, 0, sizeof(*op));
    res = cl_process(op, boxname);
    if (res)
        return SG_LIB_SYNTAX_ERROR;
    sg_fd = sg_cmds_open_device(op->dev_name, op->o_readonly, op->verbose);
    if (sg_fd < 0) {
        printf("open error: %s\n", op->dev_name);
        return SG_LIB_FILE_ERROR;
    }
    ret = join_work(sg_fd, op, 1, sas_addr, num);
err_out:
    res = sg_cmds_close_device(sg_fd);
    if (res < 0) {
        if (0 == ret)
            return SG_LIB_FILE_ERROR;
    }
    return (ret >= 0) ? 0 : -1;
}
