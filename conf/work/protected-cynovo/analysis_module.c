
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <sys/un.h>
#include <netinet/in.h>
#include "hlog.h"
#include "analysis_module.h"
#include "analysis_module_pangu.h" 


#define MIN(a, b) ((a > b) ? b : a)

static unsigned char _get_lrc(unsigned char *data, int datalen)
{
    int i;
    unsigned char lrc;

    for (i=0, lrc=0; i<datalen; i++)
        lrc = lrc^data[i];                   
 
    return lrc;
}

static void _get_tag(const char *name, unsigned char *tag)
{
    tag[0] = name[0];
    tag[1] = strlen(name);
    tag[2] = _get_lrc((unsigned char*)name, strlen(name));
}

static int _get_len(char *data)
{
    return data[0] + data[1]*256; 
}

static void _set_len(int len, char *data)
{
    data[0] = len%256;
    data[1] = len/256; 
}

int check(analysis_module_t *module)
{
    int i, len;

    for(i=0; i<module->datalen; )     
    {    
        len = _get_len(module->data +i+3);        
        if(i +3+2+len+1 >module->datalen)
            return -1;
        i += 3+2+len+1;
    }

    return 0;
}

int get(analysis_module_t *module, const char *name, unsigned char *data, int *datalen)
{
    unsigned char tag[3];
    int i, len;

    _get_tag(name, tag);
	
    //hlog(H_DEBUG, "name = <%s> \n",name);
    //hlog_data(H_DEBUG, "tag",tag, 3);

    for(i=0; i<module->datalen; )
    {    
        if(!memcmp(tag, module->data +i, 3))
        {
            len = _get_len(module->data +i+3);
			//hlog(H_DEBUG, "len = %d \n",len);
			
            if(len > *datalen)
                len = *datalen;
            *datalen = len;
            memcpy(data, module->data +i+3+2, *datalen);
            return 0;
        }

        len = _get_len(module->data +i+3);        
        if(i +3+2+len+1 >module->datalen)
            return -1;
        i += 3+2+len+1;
    }

    *datalen = 0;
    return -1;
}

int set(analysis_module_t *module, const char *name, unsigned char *data, int datalen)
{
    _get_tag(name, module->data +module->datalen);
    module->datalen += 3;

    _set_len(datalen, module->data +module->datalen);
    module->datalen += 2;

    memcpy(module->data +module->datalen, data, datalen);
    module->datalen += datalen;

    module->data[module->datalen] = _get_lrc(module->data, module->datalen);
    module->datalen += 1;

    return 0;
}

int set_string(analysis_module_t *module, const char *name, char *string)
{
    return set(module, name, (unsigned char *)string, strlen(string));
}

int convert_none(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    int outlen = inlen;
    memcpy(outdata, indata, inlen);
    return outlen;
}

int convert_timeout(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    char tmp[16] = {0};

    memcpy(tmp, indata, (inlen >sizeof(tmp))?(sizeof(tmp)-1):inlen);
    *outdata = (unsigned char)atoi(tmp);
    return 1; //module->datalen; // >>>> convert_func 返回输出数据的长度，这里应该return 1 George
}
//去掉数据中的右补0
int convert_r0_padding(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    while(indata[inlen-1] == 0x00)
    {
        inlen--;
    }
    
    int outlen = inlen;
    memcpy(outdata, indata, outlen);

    return outlen;
}

int convert_atob16(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    char tmp[16] = {0};
    int  tmpVal  = 0;

    memcpy(tmp, indata, MIN(inlen, 15));
    tmpVal = atoi(tmp);

    outdata[0] = (unsigned char)(tmpVal % 256);
    outdata[1] = (unsigned char)(tmpVal / 256);

    return 2;
}

int convert_b16toa(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    int tmpVal = indata[1]*256 + indata[0];
    return sprintf(outdata, "%d", tmpVal);
}

int convert_atob32(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    char tmp[16] = {0};
    int  tmpVal  = 0;

    memcpy(tmp, indata, MIN(inlen, 15));
    tmpVal = atoi(tmp);

    memcpy(outdata, &tmpVal, 4);

    return 4;
}

int convert_b32toa(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    unsigned int tmpVal = *((unsigned int*)indata);
    return sprintf(outdata, "%d", tmpVal);
}

int convert_pack16bLen(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    outdata[0] = (unsigned char)(inlen % 256);
    outdata[1] = (unsigned char)(inlen / 256);
    memcpy(outdata+2, indata, inlen);

    return inlen+2;
}

int convert_unpack16bLen(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    //这里inlen应当为0
    int dataLen = indata[1]*256 + indata[0];
    memcpy(outdata, indata+2, dataLen);

    return dataLen;
}

int convert_amount(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    int i = 0;

    //indata 必需为12
    for(i = 0; i < 6; i++)
    {
        outdata[i]  = (indata[i*2]   & 0x0f) << 4;
        outdata[i] |= (indata[i*2+1] & 0x0f);
    }

    return 6;
}
int convert_filename(analysis_module_t *module, unsigned char *indata, int inlen, unsigned char *outdata)
{
    memset(outdata, 0x00, 64);
    memcpy(outdata, indata, MIN(inlen, 63));

    return 64;
}

static cmd_analysis_t *_find_cmd(analysis_module_t *module, unsigned char *cmd, int cmdlen)
{
    int i;
    
    for(i=0; module->cmd[i].name; i++)
    {
        if(module->cmd[i].namelen == cmdlen &&
            !strncmp(module->cmd[i].name, cmd, cmdlen))
            return &module->cmd[i];
    }
    return 0;
}

static int _pack_tx_data(analysis_module_t *module, cmd_analysis_t *cmd, unsigned char *buf, int *buflen, char *ret_string) 
{
    param_analysis_t *param;
    unsigned char tmp[BUF_MAX_SIZE];
    int i, tmplen;
    
    *buflen = 0; 
    memcpy(buf +*buflen, cmd->key, cmd->keylen);
    *buflen += cmd->keylen;
    
    for(i=0; cmd->param[i].name; i++)
    {
        if(cmd->param[i].io != 1)
            continue;
        
        param = &cmd->param[i];
        tmplen = sizeof(tmp);            
        if(get(module, param->name, tmp, &tmplen))          
        {
            sprintf(ret_string, "tag input(%s) error", param->name);
            return -1;
        }
        else  
            *buflen += param->convert_func(module, tmp, tmplen, buf +*buflen);
    }

    return 0;
}

static int _unpack_rx_data(analysis_module_t *module, cmd_analysis_t *cmd, unsigned char *buf, int buflen, char *ret_string) 
{
    param_analysis_t *param;
    unsigned char tmp[BUF_MAX_SIZE];
    int i, len, tmplen;
    
    module->datalen = 0;
    for(i=0, len=0; cmd->param[i].name; i++)
    {
        if(cmd->param[i].io != 0)
            continue;
        param = &cmd->param[i];   
        if(len + param->datalen > buflen)
        {
            sprintf(ret_string, "tag output(%s) error", param->name);
            return -1;
        }
        tmplen = param->convert_func(module, buf +len, param->datalen, tmp);
        len += param->datalen;
        set(module, param->name, tmp, tmplen);
    }

    return 0;
}

static int _analysis_handle(analysis_module_t *module)
{
    unsigned char buf[BUF_MAX_SIZE];
    int buflen, timeout, ret = 0;
    char ret_string[64];
    cmd_analysis_t *cur_cmd;   
	int i =0;
	protected_cynovo_cmd_t * pdev = NULL;

    hlog_data(H_INFO, "analysis_handle rx", module->data, module->datalen);
	
    if(check(module))
    {
        sprintf(ret_string, "check error");
        ret = -1;
        goto EXIT;
    }

    buflen = 32;
    if(get(module, "cmd", buf, &buflen))
    {
        sprintf(ret_string, "cmd error");
        ret = -2;
        goto EXIT;
    }
	
    buf[buflen] = 0;
	hlog(H_INFO,"cmd = [%s]\n",buf);	
	
	for(i = 0; i< get_protected_cynovo_cmd_length();i++){
		pdev = get_cynovo_device(i);
		if(pdev == NULL)
			continue;
		if(pdev->cmd == NULL)
			continue;		
		if(strncmp(buf,pdev->cmd,buflen) == 0){
			ret = pdev->handlecmd(module);
			break;
		}
	}
	hlog(H_INFO,"\n\n\n");	
	
EXIT:
    hlog_data(H_INFO, "analysis_handle exit", module->data, module->datalen);
    return ret;	
}

int _analysis_socket_create(analysis_module_t *analysis_module)
{
    int s_socket;
	struct sockaddr_un s_addr;
    struct sockaddr_un *un_addr = (struct sockaddr_un *)&s_addr;
    struct sockaddr_in *in_addr = (struct sockaddr_in *)&s_addr; 
	
	if(analysis_module->socket_type == SOCKET_TYPE_LOCAL)
	{
#if defined(ANDROID_SERVICE)	
		if(!strlen(analysis_module->socket_addr))
			strncpy(analysis_module->socket_addr, ANDROID_SERVICE_SOCKET_PATH, sizeof(analysis_module->socket_addr));
		
		s_socket = android_get_control_socket(analysis_module->socket_addr);
		if(s_socket < 0)
		{
			hlog(H_ERROR, "Socket error.\n");
			return -1;
		}
		
		hlog(H_DEBUG, "Android local socket addr \"%s\"\n", analysis_module->socket_addr);
#else
		s_socket = socket(AF_UNIX, SOCK_STREAM, 0);
		if(s_socket < 0)
		{
			hlog(H_ERROR, "Socket error.\n");
			return -1;
		}
		
		if(!strlen(analysis_module->socket_addr))
			strncpy(analysis_module->socket_addr, SERVICE_SOCKET_PATH, sizeof(analysis_module->socket_addr));
		un_addr->sun_family = AF_UNIX;
		strcpy(un_addr->sun_path, analysis_module->socket_addr);
		unlink(analysis_module->socket_addr);
		
		hlog(H_DEBUG, "Local socket addr \"%s\"\n", analysis_module->socket_addr);
		
		if(bind(s_socket, (struct sockaddr *)&s_addr, sizeof(s_addr)) <0)
		{
			hlog(H_ERROR, "Socket bind error.\n");
			return -1;
		}
#endif				
		if(listen(s_socket, LISTEN_MAX_NUM) <0)
		{
			hlog(H_ERROR, "Socket listen error.\n");
			return -1;
		}
	}
	else if(analysis_module->socket_type == SOCKET_TYPE_TCP)
	{
		s_socket = socket(AF_INET, SOCK_STREAM, 0);
		if(s_socket < 0)
		{
			hlog(H_ERROR, "Socket error.\n");
			return -1;
		}
		
		if(!strlen(analysis_module->socket_addr))
			strncpy(analysis_module->socket_addr, SERVICE_SOCKET_PORT, sizeof(analysis_module->socket_addr));
		
		in_addr->sin_family = AF_INET; 
		in_addr->sin_addr.s_addr = htonl(INADDR_ANY); 
		in_addr->sin_port = htons(atoi(analysis_module->socket_addr)); 
		  
		hlog(H_DEBUG, "TCP socket port:%d\n", ntohs(in_addr->sin_port));
		  
		if(bind(s_socket, (struct sockaddr *)&s_addr, sizeof(s_addr)) <0)
		{
			hlog(H_ERROR, "Socket bind error.\n");
			return -1;
		}
		
		if(listen(s_socket, LISTEN_MAX_NUM) <0)
		{
			hlog(H_ERROR, "Socket listen error.\n");
			return -1;
		}
	}
	else if(analysis_module->socket_type == SOCKET_TYPE_UDP)
	{
		s_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if(s_socket < 0)
		{
			hlog(H_ERROR, "Socket error.\n");
			return -1;
		}
		
		if(!strlen(analysis_module->socket_addr))
			strncpy(analysis_module->socket_addr, SERVICE_SOCKET_PORT, sizeof(analysis_module->socket_addr));
		
		in_addr->sin_family = AF_INET; 
		in_addr->sin_addr.s_addr = htonl(INADDR_ANY); 
		in_addr->sin_port = htons(atoi(analysis_module->socket_addr)); 
		  
		hlog(H_DEBUG, "UDP socket port:%d\n", ntohs(in_addr->sin_port));
		  
		if(bind(s_socket, (struct sockaddr *)&s_addr, sizeof(s_addr)) <0)
		{
			hlog(H_ERROR, "Socket bind error.\n");
			return -1;
		}
	}
	else
		return -2;
	return s_socket;
}

int _analysis_socket_recv(int s_socket, struct sockaddr *addr, analysis_module_t *analysis_module)
{
	int a_socket, addr_size;
	struct sockaddr_un a_addr;
    struct sockaddr_un *un_addr = (struct sockaddr_un *)&a_addr;
    struct sockaddr_in *in_addr = (struct sockaddr_in *)&a_addr; 
	struct timeval timeout = {1, 0};

	
	printf("_analysis_socket_recv \n");
	
	if(analysis_module->socket_type == SOCKET_TYPE_LOCAL)
	{
		addr_size = sizeof(struct sockaddr_un);
		hlog(H_INFO,"\n\n\n");
		a_socket = accept(s_socket, (struct sockaddr *)&a_addr, &addr_size);
		if(a_socket <0)
		{
			//hlog(H_ERROR, "Socket accept error.\n");
			return -1;
		}
		
		hlog(H_INFO,"accept local socket addr:%s\n", analysis_module->socket_addr);
		setsockopt(a_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		
		analysis_module->datalen = recv(a_socket, analysis_module->data, sizeof(analysis_module->data), 0);
		hlog(H_INFO,"analysis_module->datalen = %d \n",analysis_module->datalen);

		if(analysis_module->datalen <0)
		{
			hlog(H_ERROR, "Socket read error.\n");
			close(a_socket);
			return -1;
		}	
	}
	else if(analysis_module->socket_type == SOCKET_TYPE_TCP)
	{
		addr_size = sizeof(struct sockaddr_un);
		
		a_socket = accept(s_socket, (struct sockaddr *)&a_addr, &addr_size);
		if(a_socket <0)
		{
			hlog(H_ERROR, "Socket accept error.\n");
			return -1;
		}

		hlog(H_DEBUG, "accept TCP socket %s:%d\n", inet_ntoa(in_addr->sin_addr), ntohs(in_addr->sin_port));
		setsockopt(a_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		
		analysis_module->datalen = recv(a_socket, analysis_module->data, sizeof(analysis_module->data), 0);
		if(analysis_module->datalen <0)
		{
			hlog(H_ERROR, "Socket read error.\n");
			close(a_socket);
			return -1;
		}	
	}
	else if(analysis_module->socket_type == SOCKET_TYPE_UDP)
	{
		addr_size = sizeof(struct sockaddr_un);
				
		analysis_module->datalen = recvfrom(s_socket, analysis_module->data, sizeof(analysis_module->data), 0 , in_addr, &addr_size);
		if(analysis_module->datalen <0)
		{
			hlog(H_ERROR, "recvfrom error %d.\n", errno);
			sleep(1);
			return -1;
		}
		
		hlog(H_DEBUG, "accept UDP socket %s:%d\n", inet_ntoa(in_addr->sin_addr), ntohs(in_addr->sin_port));
		memcpy(addr, &a_addr, sizeof(struct sockaddr));
		a_socket = s_socket;
	}
	
	return a_socket;
}

int _analysis_socket_send(int a_socket, struct sockaddr *addr, analysis_module_t *analysis_module)
{
	struct sockaddr_un a_addr;
	struct timeval timeout = {1, 0};
	
	if(analysis_module->socket_type == SOCKET_TYPE_LOCAL)
	{
		setsockopt(a_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
        if(send(a_socket, analysis_module->data, analysis_module->datalen, 0) <0)
            hlog(H_ERROR, "Socket write error.\n");
		close(a_socket);
	}
	else if(analysis_module->socket_type == SOCKET_TYPE_TCP)
	{
		setsockopt(a_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
        if(send(a_socket, analysis_module->data, analysis_module->datalen, 0) <0)
            hlog(H_ERROR, "Socket write error.\n");	
		close(a_socket);
	}
	else if(analysis_module->socket_type == SOCKET_TYPE_UDP)
	{
		if(sendto(a_socket, analysis_module->data, analysis_module->datalen, 0, (struct sockaddr*)addr, sizeof(struct sockaddr)) <0)
			hlog(H_ERROR, "Socket write error.\n");	
	}
	
	return 0;
}

void *_analysis_thread(void *data)
{
    analysis_module_t *analysis_module = (analysis_module_t *)data;        
    int s_socket, a_socket;
	struct sockaddr addr;
	struct timeval timeout={2, 0};
	
	s_socket =  _analysis_socket_create(analysis_module);
	if(s_socket < 0)
	{
        hlog(H_ERROR, "_analysis_socket_create error.\n");
		return NULL;
	}
	printf("Begin to accept socket client\n");

    while(1)
    {   
    
		analysis_module->datalen = 0;
		
        a_socket = _analysis_socket_recv(s_socket, &addr, analysis_module);
		hlog(H_INFO,"a_socket = %d \n",a_socket);
		
        if(a_socket <0)
            continue;
		
        if(analysis_module->data[analysis_module->datalen-1] == 0x0a)
            analysis_module->datalen--;
     
        _analysis_handle(analysis_module);

        if(_analysis_socket_send(a_socket, &addr, analysis_module))
            hlog(H_INFO,"Socket write error.\n");
    }   

    close(s_socket);

    return NULL;
}

void analysis_module_init(analysis_module_t *module)
{
    static pthread_t thread;
    pthread_create(&thread, NULL, _analysis_thread, module);
}
