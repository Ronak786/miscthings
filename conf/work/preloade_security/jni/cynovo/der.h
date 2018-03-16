#ifndef __DER_H__
#define __DER_H__


typedef enum
{
    BOOLEAN = 0x01,
    INTERGER = 0x02,
    BIT_STRING = 0x03,
    OCTET_STRING = 0x04,
    TAG_NULL = 0x05,
    OBJECT_ID = 0x06,
    OBJECT_DESC = 0x07,
    REAL = 0x08,
    ENUMERATED = 0x0a,
    UTF8STRING = 0x0c,
    SEQUENCE = 0x10,
    SEQUENCE_APP = 0x20|SEQUENCE,
    SET = 0x11,
    SET_APP = 0x20|SET,
    NUMERICSTRING = 0x12,
    PRINTABLESTRING = 0x13,
    T61STRING = 0x14,
    IA5STRING = 0x16,
    UTCTIME = 0x17,
    //added by eddard
    CERTVER=0xa0,
    EXTENSION=0xa3,
	KEYID=0x80,
	SERIALNO=0x82,
}DerTag_t;

typedef struct 
{
    DerTag_t tag;
    uint8_t *tagData;
    uint32_t tagDataLen;
}DerTLV_t;

#endif


