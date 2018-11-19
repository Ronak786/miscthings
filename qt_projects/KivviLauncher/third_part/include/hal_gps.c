#include "hal_gps.h"
#include "hal_uart.h"

#include <string.h>

#define DEV_GPS     "/dev/ttySAC1"

static UartInfo_t   *_UartInfo;
static GpsInfo_t    _GpsInfo;

static uint8_t _GpsBuffer[1024];

int HAL_GpsOpen(void)
{
    _UartInfo = HAL_UartOpen(DEV_GPS);

    if (_UartInfo->fd < 0)
    {
        HAL_LogError("HAL_GpsOpen() fail, _fd: %d", _UartInfo->fd);
        HAL_GpsClose();
        return -1;
    }

    /* Set baudrate */

    HAL_LogInfo("HAL_GpsOpen() succ");

    return _UartInfo->fd;
}

int HAL_GpsClose(void)
{
    close(_UartInfo->fd);

    HAL_LogInfo("HAL_GpsClose() succ");

    return 0;
}

int HAL_GpsRead(void)
{
    int i = 0;
    char c;
    char buf[1024];
    volatile uint8_t t = 1;

    while (t)
    {
        if (read(DEV_GPS, &c, 1) < 0 )
        {
            HAL_LogInfo("HAL_GpsRead Error");
            return -1;
        }
        
        buf[i++] = c;
        if (c == '\n')
        {
            strncpy(_GpsBuffer, buf, i);
            i = 0;
            t = 0;
        }
    }

    return 0;
}


static int GetComma(int num, char *str)
{
    int i, j = 0;
    int len = strlen(str);
    for (i= 0; i < len; i++)
    {
        if (str[i] == ',')
            j++;
        if (j == num)
            return i + 1;   //���ص�ǰ�ҵ��Ķ���λ�õ���һ��λ��
    }

    return 0;	
}

static double get_double_number(char *s)
{
    char buf[128];
    int i;
    double rev;
    i = GetComma(1, s);
    strncpy(buf, s, i);
    buf[i] = 0;
    rev = atof(buf);

    return rev;
}

static void UTC2BTC(DateTime_t *dt)
{
    //�������ȳ�,�ٳ�ʱ������,��ʱ������+1��
    dt->second++; //��һ��
    if (dt->second > 59)
    {
        dt->second = 0;
        dt->minute++;
        if (dt->minute > 59)
        {
            dt->minute = 0;
            dt->hour++;
        }
    }	
 
    dt->hour += 8;      //����ʱ���UTCʱ�����8Сʱ
    if (dt->hour > 23)
    {
        dt->hour -= 24;
        dt->day += 1;

        if ((dt->month == 2) || (dt->month == 4) || (dt->month == 6) || (dt->month == 9) || (dt->month == 11))
        {
            if(dt->day > 30)
            {
                //���������·���30��ÿ�£�2�·ݻ�����30
                dt->day=1;
                dt->month++;
            }
        }
        else
        {
            if (dt->day > 31)
            {
                //ʣ�µļ����·ݶ���31��ÿ��
                dt->day = 1;
                dt->month++;
            }
        }

        if (dt->year % 4 == 0)
        {
            if ((dt->day > 29) && (dt->month == 2))
            {
                //����Ķ�����29��
                dt->day=1;
                dt->month++;
            }
        }
        else
        {
            if ((dt->day > 28) && (dt->month == 2))
            {
                //�����Ķ�����28��ÿ��
                dt->day = 1;
                dt->month++;
            }
        }

        if (dt->month > 12)
        {
            dt->month -= 12;
            dt->year++;
        }		
    }
}

void HAL_GpsInfoDecode(void)
{
    int i, tmp, start, end;
    char c;
    //char* buf = line;
    c = _GpsBuffer[5];

    if (c == 'C')
    {
        //"GPRMC"
        _GpsInfo.Dt.hour   = (_GpsBuffer[ 7] - '0') * 10 + (_GpsBuffer[ 8] - '0');
        _GpsInfo.Dt.minute = (_GpsBuffer[ 9] - '0') * 10 + (_GpsBuffer[10] - '0');
        _GpsInfo.Dt.second = (_GpsBuffer[11] - '0') * 10 + (_GpsBuffer[12] - '0');
        tmp = GetComma(9, _GpsBuffer);                                         //�õ���9�����ŵ���һ�ַ����
        _GpsInfo.Dt.day    = (_GpsBuffer[tmp+0] - '0') * 10 + (_GpsBuffer[tmp+1] - '0');
        _GpsInfo.Dt.month  = (_GpsBuffer[tmp+2] - '0') * 10 + (_GpsBuffer[tmp+3] - '0');
        _GpsInfo.Dt.year   = (_GpsBuffer[tmp+4] - '0') * 10 + (_GpsBuffer[tmp+5] - '0') + 2000;
        //------------------------------
        _GpsInfo.Status     = _GpsBuffer[GetComma(2, _GpsBuffer)];                        //״̬
        _GpsInfo.Latitude   = get_double_number(&_GpsBuffer[GetComma(3, _GpsBuffer)]);    //γ��
        _GpsInfo.NorthSouth = _GpsBuffer[GetComma(4, _GpsBuffer)];                        //�ϱ�γ
        _GpsInfo.Longitude  = get_double_number(&_GpsBuffer[GetComma(5, _GpsBuffer)]);    //����
        _GpsInfo.EastWest   = _GpsBuffer[GetComma(6, _GpsBuffer)];                        //������
        UTC2BTC(&_GpsInfo.Dt);                                              //ת����ʱ��
    }

    if (c == 'A')
    {
        //"$GPGGA"
        _GpsInfo.High = get_double_number(&_GpsBuffer[GetComma(9, _GpsBuffer)]);
    }
}

void HAL_GpsInfoPrint(void)
{
    printf("DATE     : %ld-%02d-%02d \n", _GpsInfo.Dt.year, _GpsInfo.Dt.month, _GpsInfo.Dt.day);
    printf("TIME     : %02d:%02d:%02d \n", _GpsInfo.Dt.hour, _GpsInfo.Dt.minute, _GpsInfo.Dt.second);
    printf("Latitude : %10.4f %c\n", _GpsInfo.Latitude, _GpsInfo.NorthSouth);
    printf("Longitude: %10.4f %c\n", _GpsInfo.Longitude, _GpsInfo.EastWest);
    printf("high     : %10.4f \n", _GpsInfo.High);
    printf("STATUS   : %c\n", _GpsInfo.Status);
}
