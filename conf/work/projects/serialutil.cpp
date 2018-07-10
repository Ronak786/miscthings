#include "debug.h"
#include <atlbase.h>
#include <vector>
#include <string>

HANDLE  hComm;

int GetList(std::vector<std::string>& list);
BOOL SendDataToUartPort(HANDLE hComm, void *SndBuf, DWORD SendDataLen, DWORD *SentDataLen, LPOVERLAPPED lpOverlapped);
BOOL OpenUartPort(char *UartPortName);
BOOL InitUartPort(HANDLE hComm, DWORD BaudRate, BYTE ByteSize, BYTE Parity, BYTE StopBits);
BOOL RcvDataFromUartPort(HANDLE hComm, void *RcvBuf, DWORD ToRcvDataLen, DWORD *RcvedDataLen, LPOVERLAPPED lpOverlapped);
BOOL SendDataToUartPort(HANDLE hComm, void *SndBuf, DWORD SendDataLen, DWORD *SentDataLen, LPOVERLAPPED lpOverlapped);
void Close_UartPort();
BOOL getUart(std::string &name);

int main() {
    dprintf("begin here\n");

    std::vector<std::string> mlist;
    int count = GetList(mlist);
    dprintf("count is %d\n", count);
    for (int i = 0; i < count; ++i) {
        dprintf("dev: %s\n", mlist[i].c_str());
    }

    std::string tmpstr;
    getUart(tmpstr);
    return 0;


    char *tmpname = "com";
    if (OpenUartPort(const_cast<char*>(tmpname)) == FALSE) {
        dprintf("error open\n");
        return -1;
    }
    char sbuf[30] = "RP 1111\r\n";
    char rbuf[1024];
    DWORD slen = 65536, rlen = 65536;
    
    if (SendDataToUartPort(hComm, sbuf, strlen(sbuf), &slen, NULL) == FALSE) {
        dprintf("can not send\n");
        return -1;
    }
    dprintf("send length is %lu\n", slen);

    while (rlen > 1000 || rlen == 0) {
        dprintf("trying\n");
        if (RcvDataFromUartPort(hComm, rbuf, 128, &rlen, NULL) == FALSE) {
            dprintf("can not receive\n");
            return -1;
        }
        Sleep(1000);
    }

    dprintf("rlen is %lu\n", rlen);
    rbuf[rlen] = '\0';
    dprintf("received :%s:\n", rbuf);
    Close_UartPort();
    return 0;
}

BOOL getUart(std::string &name) {
    // from 0 to 100 iterate
    char rbuf[256];
    for (int i = 0; i < 100; ++i) {
        std::string tmpname = "\\\\.\\COM" + std::to_string(i);
        if (OpenUartPort(const_cast<char*>(tmpname.c_str())) == TRUE) {
            dprintf("%d opened\n", i);
            DWORD slen = 65536, tries = 3, rlen = 65536;
            
            SendDataToUartPort(hComm, "1", 1, &slen, NULL);
            while (--tries != 0) {
                RcvDataFromUartPort(hComm, rbuf, sizeof(rbuf), &rlen, NULL);
                if (rlen < 1000 && rlen != 0) {
                    break;
                }
                Sleep(300);
            }
            if (rlen > 1000 || rlen == 0) {
                dprintf("drop this com, continue\n");
                Close_UartPort();
                continue;
            }
            else {
                dprintf("get com port :%s:\n", tmpname.c_str());
                name = tmpname;
                return TRUE;
            }
        }
    }
    return TRUE;
}

int GetList(std::vector<std::string>& list)
{
    CRegKey RegKey;
    int nCount = 0;

    if (RegKey.Open(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM") == ERROR_SUCCESS)
    {
        dprintf("inside loop outer\n");
        while (true)
        {
            char ValueName[_MAX_PATH];
            unsigned char ValueData[_MAX_PATH];
            DWORD nValueSize = _MAX_PATH;
            DWORD nDataSize = _MAX_PATH;
            DWORD nType;
            if (RegEnumValue(HKEY(RegKey), nCount, ValueName, &nValueSize, NULL, &nType, ValueData, &nDataSize) == ERROR_NO_MORE_ITEMS)
            {
                break;
            }
            list.push_back((char*)ValueData);
            nCount++;
        }
    }

    return nCount;
}


BOOL OpenUartPort(char *UartPortName)
{
    BOOL bResult = true;
    hComm = CreateFile(
        UartPortName,
        GENERIC_WRITE | GENERIC_READ,   //访问权限
        0,                          //不共享
        NULL,                           //返回的句柄不允许被子进程继承
        OPEN_EXISTING,
        0,                          //0：同步模式，FILE_FLAG_OVERLAPPED：异步模式
        0                               //不使用临时文件句柄
    );
    if (INVALID_HANDLE_VALUE == hComm)
    {
//      dprintf("Open Failed!!!\n");
        bResult = false;
    }
    else
    {
        dprintf("Open Successfully!!!\n");
        if (InitUartPort(hComm, 19200, 8, NOPARITY, ONESTOPBIT))
        {
//          dprintf("Init Uart Port OK!!!\n");
        }
        else
        {
            //PrintLogCom("Init Uart Port Failed!!!\n");
            bResult = false;
            Close_UartPort();
        }
    }

    return bResult;
}

//Init UART
BOOL InitUartPort(HANDLE hComm, DWORD BaudRate, BYTE ByteSize, BYTE Parity, BYTE StopBits)
{
    BOOL bResult = true;
    char buffer[50] = "";

    //设置接收缓冲区和输出缓冲区的大小
    DWORD dwInQueue = 1024;
    DWORD dwOutQueue = 1024;
    if (!SetupComm(hComm, dwInQueue, dwOutQueue))
    {
        //PrintLogCom("Set In and Out Buffer Failed!!!\n");
        bResult = false;
    }
    else
    {
        //PrintLogCom("Set Input and Output Buffer OK!!!\n");

        //设置读写的超时时间  以毫秒计算
        COMMTIMEOUTS timeouts;
        //for read ReadTotalTimeouts = ReadTotalTimeoutMultiplier * ToReadByteNumber + ReadTotalTimeoutConstant，
        timeouts.ReadIntervalTimeout = MAXDWORD;                    //接收两个字符之间的最长超时时间
        timeouts.ReadTotalTimeoutMultiplier = 0;            //与读取要读字节数相乘的系数
        timeouts.ReadTotalTimeoutConstant = 0;              //读取总超时时间常量
                                                            //for write WriteTotalTimeouts = WriteTotalTimeoutMultiplier * ToWriteByteNumber + WriteTotalTimeoutConstant
                                                            //timeouts.WriteTotalTimeoutMultiplier = 0;
                                                            //timeouts.WriteTotalTimeoutConstant = 0;

        if (!SetCommTimeouts(hComm, &timeouts))
        {
            //PrintLogCom("Set Read/Write Timeouts Failed!!!\n");
            bResult = false;
        }
        else
        {
            //PrintLogCom("Set Read/Write Timeouts OK!!!\n");
            //设置DCB(Device-Control-Block)
            DCB dcb;
            if (!GetCommState(hComm, &dcb))
            {
                //PrintLogCom("Get Com DCB Failed!!!\n");
                bResult = false;
            }
            else
            {
                //PrintLogCom("Get Com DCB Successfully!!!\n");
                /*
                sprintf(buffer,"BaudRate = %ld",dcb.BaudRate);
                PrintLogCom(buffer);
                sprintf(buffer,"ByteSize = %u\n",dcb.ByteSize);
                PrintLogCom(buffer);
                sprintf(buffer,"Parity = %u\n",dcb.Parity);
                PrintLogCom(buffer);
                sprintf(buffer,"StopBits = %u\n",dcb.StopBits);
                PrintLogCom(buffer);
                sprintf(buffer,"XonChar = %d\n", dcb.XonChar);
                PrintLogCom(buffer);
                */
                memset(&dcb, 0, sizeof(dcb));
                dcb.BaudRate = BaudRate;
                dcb.ByteSize = ByteSize;
                dcb.Parity = Parity;
                dcb.StopBits = StopBits;
//              dcb.XonChar = 1;

                if (!SetCommState(hComm, &dcb))
                {
                    //PrintLogCom("Set Com Failed!!!\n");
                    bResult = false;
                }
                else
                {
                    //PrintLogCom("Set Com Successfully!!!\n");

                    //清空接收缓存和输出缓存的buffer
                    if (!PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR))
                    {
                        //PrintLogCom("Clean up in buffer and out buffer Failed!!!\n");
                        bResult = false;
                    }
                    else
                    {
                        //PrintLogCom("Clean up in buffer and out buffer OK!!!\n");
                    }
                }
            }
        }
    }

    return bResult;
}

BOOL RcvDataFromUartPort(HANDLE hComm, void *RcvBuf, DWORD ToRcvDataLen, DWORD *RcvedDataLen, LPOVERLAPPED lpOverlapped)
{
    BOOL bResult = true;
    DWORD dwTempRcvedDataLen = 0;
    DWORD dwError;

    if (ClearCommError(hComm, &dwError, NULL))
    {
        PurgeComm(hComm, PURGE_TXABORT | PURGE_TXCLEAR);
    }

    if (hComm != INVALID_HANDLE_VALUE)
    {
        if (!ReadFile(hComm, RcvBuf, ToRcvDataLen, &dwTempRcvedDataLen, lpOverlapped))
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                while (!GetOverlappedResult(hComm, lpOverlapped, &dwTempRcvedDataLen, FALSE))
                {
                    if (GetLastError() == ERROR_IO_INCOMPLETE)
                    {
                        continue;
                    }
                    else
                    {
                        ClearCommError(hComm, &dwError, NULL);
                        bResult = false;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        bResult = false;
    }
    *RcvedDataLen = dwTempRcvedDataLen;
    return bResult;
}

BOOL SendDataToUartPort(HANDLE hComm, void *SndBuf, DWORD SendDataLen, DWORD *SentDataLen, LPOVERLAPPED lpOverlapped)
{
    BOOL bResult = true;
    DWORD dwTempSndDataLen;
    DWORD dwError;

    if (ClearCommError(hComm, &dwError, NULL))
    {
        PurgeComm(hComm, PURGE_TXABORT | PURGE_TXCLEAR);
    }

    if (hComm != INVALID_HANDLE_VALUE)
    {
        if (!WriteFile(hComm, SndBuf, SendDataLen, &dwTempSndDataLen, lpOverlapped))
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                while (!GetOverlappedResult(hComm, lpOverlapped, &dwTempSndDataLen, FALSE))
                {
                    if (GetLastError() == ERROR_IO_INCOMPLETE)
                    {
                        continue;
                    }
                    else
                    {
                        ClearCommError(hComm, &dwError, NULL);
                        bResult = false;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        bResult = false;
    }
    *SentDataLen = dwTempSndDataLen;

    return bResult;
}

void Close_UartPort() {
    CloseHandle(hComm);
}

