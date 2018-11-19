#ifndef __HAL_GPS_H__
#define __HAL_GPS_H__

#include <stdint.h>
#include "hal_def.h"

typedef struct
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} DateTime_t;

typedef struct
{
    DateTime_t  Dt;         /* !< Date time */
    uint8_t     Status;     /* !< Receive status */
    double      Latitude;   /* !< Latitude */
    double      Longitude;  /* !< Longitude */
    uint8_t     NorthSouth; /* !< North and South */
    uint8_t     EastWest;   /* !< East and West */
    double      Speed;      /* !< Speed */
    double      High;       /* !< High */
} GpsInfo_t;

int HAL_GpsOpen(void);

int HAL_GpsClose(void);

int HAL_GpsRead(void);

void HAL_GpsInfoDecode(void);

void HAL_GpsInfoPrint(void);

#endif  /* __HAL_GPS_H__ */
