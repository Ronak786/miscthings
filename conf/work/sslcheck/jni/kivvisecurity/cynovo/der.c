#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "der.h"


static uint8_t *_GetTag(uint8_t *buf, DerTag_t *tag)
{
    uint8_t *p = buf;
    
    *tag = (DerTag_t)*p;    
    return p +1;
}

static uint8_t *_GetLength(uint8_t *buf, uint32_t *length)
{
    uint8_t *p = buf;

    switch(p[0])
    {
        case 0x81:
            *length = p[1];
            p += 1+1;
            break;
        case 0x82:
            *length = p[1]*0x100 +p[2];
            p += 1+2;
            break;
        case 0x83:
            *length = p[1]*0x10000 +p[2]*0x100 +p[3];
            p += 1+3;
            break;
        default:
            if(p[0] < 0x80)
            {
                *length = p[0];
                p += 1;
                break;
            } 
            p = NULL; 
            break;
    }
    return p; 
} 

static uint8_t *_GetValue(uint8_t *buf, uint8_t *value, uint8_t **valueAddr, uint32_t valueLen)
{
    uint8_t *p = buf;
   
    if(value) 
        memcpy(value, p, valueLen);
    if(valueAddr)
        *valueAddr = p;

    return p +valueLen;
}

static uint8_t *DER_GetTLV(uint8_t *pBuf, DerTag_t *pTag, uint8_t *pValue, 
	uint8_t **ppValueAddr, uint32_t *pValueLen)
{
    uint8_t *p;
    DerTag_t tag;
    uint32_t valueLen;

    p = _GetTag(pBuf, &tag);
    p = _GetLength(p, &valueLen);
    if(!p)
        return p;
    p = _GetValue(p, pValue, ppValueAddr, valueLen);
    if(pTag)
        *pTag = tag;
    if(pValueLen)
        *pValueLen = valueLen;
    return p;
}

static uint8_t * _SetTag(uint8_t *pBuf, DerTag_t tag)
{
    uint8_t *p = pBuf;
	
    *p = tag;	
	p += 1;
	
    return p;
}

static uint8_t *_SetLength(uint8_t *pBuf, uint32_t length)
{
    uint8_t *p = pBuf;
	
    if(length < 0x80)
    {
        *p = length;
        p += 1;		
    }
    else if(length <= 0xff)
    {
        p[0] = 0x81;
        p[1] = length;
        p += 2;
    }
    else if(length <= 0xffff)
    {
        p[0] = 0x82;
        p[1] = length>>8;
        p[2] = length;
        p += 3;
    }
    else if(length <= 0xffffff)
    {
        p[0] = 0x83;
        p[1] = length >>16;
        p[2] = length >>8;
        p[3] = length;
        p += 4;
    }
    return p;
} 

static uint8_t *_SetValue(uint8_t *pBuf, uint8_t *value, uint32_t valueLen)
{
    uint8_t *p = pBuf;
    memcpy(p, value, valueLen);
    p += valueLen;

    return p;
}


uint8_t * DER_SetTLV(uint8_t *pBuf, DerTag_t tag, uint8_t *value, uint32_t valueLen)
{
    uint8_t *p = pBuf;

    p = _SetTag(p, tag);
    p = _SetLength(p, valueLen);
    p = _SetValue(p, value, valueLen);
	
    return p;
}

uint32_t DER_ReadTLV(uint8_t *data, uint32_t dataLen,int id,DerTLV_t *tlvtemp)
{
    uint8_t *p, *addr;
    uint32_t len;
	int  ret = -1;	
	DerTag_t tag;
	
    p = data;
    while(p < data +dataLen-1)
    {	
        p = DER_GetTLV(p, &tag, 0, &addr, &len);
        if(NULL == p)
            break;

		if(id==0){	
			ret = 0;
			tlvtemp->tag=tag;
			tlvtemp->tagDataLen = len;
			tlvtemp->tagData = addr;
			break;
		}
		id--;
    }
    return ret;
}


int DER_FindTLV(DerTLV_t *tlv, uint8_t *data, uint32_t dataLen, uint8_t loop)
{
    int i, ret = -1;
    uint8_t *p, *addr;
    DerTag_t tmpTag;
    uint32_t len;

    p = data;
    while(p < data +dataLen )
    {
    
        p = DER_GetTLV(p, &tmpTag, 0, &addr, &len);
        if(NULL == p)
            break;
		
		dump("tag[%02x]\n",tmpTag);
		dump(addr, len);
	
        switch(tmpTag)
        {
            case SEQUENCE:
            case SEQUENCE_APP:
            case SET:
            case SET_APP:
                ret = DER_FindTLV(tlv, addr, len, loop+1); 
                break;
            default:
                break;
        }
        if(!ret)
            break;
    }
    return ret;
}
