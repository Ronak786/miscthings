#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

#define CMD_LEN_MAX 128

//#define DEMO_PRINT(fm, ...)  printf(fm, ##__VA_ARGS__)
#define DEMO_PRINT(fm, ...)  
//#define DEMO_PRINT printf

#define REPLY_OK                  "OK"
#define REPLY_CONNECT       "CONNECT"
#define REPLY_ERROR           "CME ERROR"

static pthread_t sRcvThreadSim0;

static pthread_mutex_t SetCmdMutex;
static pthread_mutex_t GetCmdMutex;

static void* reportAnalysisHandleSim0(void* param);
static void* detectModemStatus(void* param);


static int sFdSim0_0 = -1;
static int sFdSim0_1 = -1;
static int sFdSim0_2 = -1;

int send_atcmd(int stty_fd, char *at_str)
{
    int ret = -1;
    int count = 0, length = 0;
    char buffer[256] = {0};
    fd_set rfds;
    struct timeval timeout;

    length = strlen(at_str);

    for (;;)
    {
        ret = write(stty_fd, at_str, length);
        if (ret != length)
        {
            close(stty_fd);
            DEMO_PRINT("system broken\n");
        }

WAIT_RESPOSE_DATA:
        timeout.tv_sec=1;
        timeout.tv_usec=0;
        FD_ZERO(&rfds);
        FD_SET(stty_fd, &rfds);

        ret = select(stty_fd + 1, &rfds, NULL, NULL, &timeout);
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            else
            {
                close(stty_fd);
                DEMO_PRINT("system broken\n");
                exit(-1);
            }
        }
        else if (ret == 0)
        {
            DEMO_PRINT("select timeout");
            continue;
        }
        else
        {
            memset(buffer, 0, sizeof(buffer));
            count = read(stty_fd, buffer, sizeof(buffer)-1);
            if (count <= 0)
            {
                continue;
            }

            if (strstr(buffer, "OK"))
            {
                DEMO_PRINT("rcv ok\n");
                ret = 1;
                break;
            }
            else if (strstr(buffer, "ERROR"))
            {
                DEMO_PRINT("wrong modem state, exit!");
                close(stty_fd);
                exit(-1);
            }
            else
            {
                //go on wait
                DEMO_PRINT("do nothing, go on wait!");
                goto WAIT_RESPOSE_DATA;
            }
        }
    }

    return ret;
}

//?????????·?
int GeneralCommand(char* cmd, int timeout)
{
    int ret = 0;
    char rcvBuf[512] = {0};
    fd_set myrdfd;
    struct timeval time = {0};

    int optionSetFd = sFdSim0_1;

    DEMO_PRINT("modemDemo:%s\n",cmd);
    pthread_mutex_lock(&SetCmdMutex);

    if(optionSetFd<0)
    {
        DEMO_PRINT("modemDemo:optionSetFd < 0\n");
        ret = -1;
        goto quick_exit;
    }

    ret = write(optionSetFd,cmd,strlen(cmd));
    if(ret < 0)
    {
        DEMO_PRINT("modemDemo:write < 0\n");
        ret = -2;
        goto quick_exit;
    }

READ_AGAIN:
    FD_ZERO(&myrdfd);
    time.tv_sec = timeout;
    time.tv_usec = 0;
    FD_SET(optionSetFd,&myrdfd);

    ret = select (optionSetFd + 1, &myrdfd, NULL, NULL, &time);

    if(ret < 0)
    {
        DEMO_PRINT("modemDemo:read timeout\n");
        ret = -3;
       goto quick_exit;
    }

    memset(rcvBuf,0,512);

    ret = read(optionSetFd, rcvBuf, 512);

    if(ret > 0)
    {
        if(strstr(rcvBuf, REPLY_OK)!=0)
        {
            ret = 0;
            DEMO_PRINT("success done ok\n");
            goto quick_exit;
        }

        if(strstr(rcvBuf, REPLY_CONNECT)!=0)
        {
            ret = 0;
            DEMO_PRINT("success connect ok\n");
            goto quick_exit;
        }

        if(strstr(rcvBuf, REPLY_ERROR)!=0)
        {
            ret = 0;
            DEMO_PRINT("CMD ERR: %s\n",rcvBuf);
            goto quick_exit;
        }

        DEMO_PRINT("UNKNOW REPLAY: %s\n",rcvBuf);
        goto READ_AGAIN;
    }
    else
    {
        DEMO_PRINT("modemDemo:read timeout\n");
        ret = -4;
        goto quick_exit;
    }

quick_exit:
        pthread_mutex_unlock(&SetCmdMutex);

    return ret;
}

int infoGetCommand(char* cmd, int timeout,char* buf, int len)
{
    int ret = 0;
    fd_set myrdfd;
    struct timeval time = {0};

    int infoGetFd = sFdSim0_2;

    DEMO_PRINT("modemDemo:%s\n",cmd);
    pthread_mutex_lock(&GetCmdMutex);

    if(infoGetFd<0)
    {
        DEMO_PRINT("modemDemo:optionSetFd < 0\n");
        ret = -1;
        goto quick_exit;
    }

    FD_ZERO(&myrdfd);
    time.tv_sec = timeout;
    time.tv_usec = 0;
    FD_SET(infoGetFd,&myrdfd);

    ret = write(infoGetFd,cmd,strlen(cmd));
    if(ret < 0)
    {
        DEMO_PRINT("modemDemo:write < 0\n");
        ret = -2;
        goto quick_exit;
    }

    ret = select (infoGetFd + 1, &myrdfd, NULL, NULL, &time);

    if(ret < 0)
    {
        DEMO_PRINT("modemDemo:read timeout\n");
        ret = -3;
       goto quick_exit;
    }

    ret = read(infoGetFd, buf, len);

quick_exit:
    pthread_mutex_unlock(&GetCmdMutex);
    return ret;
}

//read me:?????ϱ?ͨ?��???ȥ??ȡ???գ???һ????buf(2048byte),??buf????????ϵͳ???쳣??
int main(int argc ,void** argv)
{
    char infoData[2048] = {0};
    char cmdStr[CMD_LEN_MAX] = {0};
    pthread_attr_t attr;
    int count = 3;

    DEMO_PRINT("Entry AmoiTest\r\n");

    pthread_mutex_init(&SetCmdMutex,NULL);
    pthread_mutex_init(&GetCmdMutex,NULL);

    //sim0 ?????ϱ?ͨ??
    sFdSim0_0 = open ("/dev/CHNPTYL0", O_RDWR);

    if(sFdSim0_0 <=0)
    {
        DEMO_PRINT("system broken ,exit\n");
        exit(-1);
    }

/*
    //cmux init
    strcpy(cmdStr, "AT+SMMSWAP=0\r");
    send_atcmd(sFdSim0_0, cmdStr);
    strcpy(cmdStr, "AT+CMUX=0\r");
    send_atcmd(sFdSim0_0, cmdStr);
*/

    //sim0 atͨ?? ??Ϊat????????ͨ??
     sFdSim0_1 = open ("/dev/CHNPTYL1", O_RDWR);
    if( sFdSim0_1 <= 0)
    {
        DEMO_PRINT("open /dev/CHNPTYL1 error\r\n");
        exit(-1);
    }

    //sim0 atͨ?? ??Ϊat??ѯ????ͨ??
     sFdSim0_2 = open ("/dev/CHNPTYL2", O_RDWR);
    if( sFdSim0_2 <= 0)
    {
        DEMO_PRINT("open /dev/CHNPTYL2 error\r\n");
        exit(-1);
    }


    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&sRcvThreadSim0, &attr, reportAnalysisHandleSim0, NULL);

/*
    //??ʼ??????
    strcpy(cmdStr, "ATE0Q0V1\r");
    GeneralCommand(cmdStr,2);

    strcpy(cmdStr, "AT+CCED=2,8\r");
    GeneralCommand(cmdStr,2);

    strcpy(cmdStr, "AT+CREG=0\r");
    GeneralCommand(cmdStr,5);

    strcpy(cmdStr, "AT+CEREG=1\r");
    GeneralCommand(cmdStr,5);

    strcpy(cmdStr, "AT+CEMODE=1\r");
    GeneralCommand(cmdStr,2);
*/

    strcpy(cmdStr, "AT+SPSWITCHDATACARD=1,0\r");
    GeneralCommand(cmdStr,2);

    strcpy(cmdStr, "ATS0=1\r");
    GeneralCommand(cmdStr,2);

    strcpy(cmdStr, "AT+SFUN=5\r");
    GeneralCommand(cmdStr,50);

    strcpy(cmdStr, "AT+SPTESTMODEM=1,254\r");
    GeneralCommand(cmdStr,2);

    strcpy(cmdStr, "AT+SFUN=4\r");
    GeneralCommand(cmdStr,50);

    //??ѯcereg ע???Ϻ???ִ?в???
GET_CEREG_STATE:
    sleep(2);
    strcpy(cmdStr, "AT+CEREG?\r");
    memset(infoData,0,2048);
    infoGetCommand(cmdStr,5,infoData,2048);

    if(strstr(infoData, "+CEREG: 2,1") ==0)
    {
	if (--count) {
		goto GET_CEREG_STATE;
	} else {
		printf("card not registerd\n");
		return -1;
	}
    }

    strcpy(cmdStr, "AT+CGDCONT=1,\"ip\",\"cmnet\"\r");
    GeneralCommand(cmdStr,5);

    strcpy(cmdStr, "AT+CGPCO=0,\"name\",\"password\",1,1\r");
    GeneralCommand(cmdStr,5);

    strcpy(cmdStr, "AT+CGEQREQ=1,2,0,0,0,0,2,0,\"1e4\",\"0e0\",3,0,0\r");
    GeneralCommand(cmdStr,5);

    strcpy(cmdStr, "AT+SPREATTACH\r");
    GeneralCommand(cmdStr,120);

    sleep(2);
    strcpy(cmdStr, "AT+CGDATA=\"M-ETHER\",1\r");
    GeneralCommand(cmdStr,120);

    strcpy(cmdStr, "AT+CGCONTRDP=1\r");
    memset(infoData,0,2048);
    infoGetCommand(cmdStr,10,infoData,2048);

    printf("%s",infoData);

/*
    while(1)
    {
        pause();
    }
*/

    return 0;
}

static void* reportAnalysisHandleSim0(void* param)
{
    char rcvData[1024] = {0};
    int rspLen = 0;

    if(sFdSim0_0<0)
        return 0;

    DEMO_PRINT("AmoiTest Rcv0 Start\r\n");

    while(1)
    {
        memset(rcvData,0,1024);
        rspLen = read(sFdSim0_0, &rcvData, 1024);
        //DEMO_PRINT("CH0\r\n%s",rcvData);
  }

}

static void* detectModemStatus(void* param)
{
    int connect_fd;
    int ret;
    static struct sockaddr_un srv_addr;
    int try_times = 0;
    char buf[128];
    int numRead;

reconnect:
//creat unix socket
    connect_fd=socket(PF_UNIX,SOCK_STREAM,0);
    if(connect_fd<0)
    {
        DEMO_PRINT("cannot create communication socket");
        goto reconnect;
    }

    srv_addr.sun_family=AF_UNIX;
    strcpy(srv_addr.sun_path,"/persist/modemd.domain");

    //connect server
    try_times = 0;
    while(1)
    {
        ret=connect(connect_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
        if(ret==-1)
        {
            if(try_times>5)
            {
                close(connect_fd);
                DEMO_PRINT("cannot connect to the server");
                usleep(10*1000);
                goto reconnect;
            }

            try_times++;
        }
        else
        {
            break;
        }
    }

    do{
       memset(buf, 0, sizeof(buf));
       do {
          numRead = read(connect_fd, buf, sizeof(buf));
          DEMO_PRINT("read %d from modemd,erro %s", numRead,(numRead < 0 ? strerror(errno) : 0));
       } while(numRead < 0 && errno == EINTR);

       if(numRead <= 0) {
           close(connect_fd);
           goto reconnect;
       }

        DEMO_PRINT("%s: read numRead=%d, buf=%s", __func__, numRead, buf);
        if (strstr(buf, "Modem Assert") )
        {
            //modem?쳣
        }
        else if (strstr(buf, "Modem Reset") )
        {
            //modem?쳣
        }
        else
        {
            //do nothing
        }

   }while(1);

    return NULL;
}


