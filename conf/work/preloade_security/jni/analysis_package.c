#include "stdio.h"
#include "string.h"

#include "analysis_package.h"

static unsigned char _get_lrc(unsigned char *data, int datalen)
{
    int i;
    unsigned char check;

    for(i=0, check=0xff; i<datalen; i++)
        check ^= data[i];
    return check;

}

static void _get_tag(const char *name,int namelen, unsigned char *tag)
{
	int i = 0;
    tag[0] = name[0];
    tag[1] = namelen;
    tag[2] = 0;
	for(i= 0; i < namelen; i++){
		tag[2] += name[i];
		tag[2]  = (unsigned char)(~tag[2]);
	}
			
}

int pack_data(unsigned char *buf,int len,
	unsigned char * name,int namelen,unsigned char* value,int valuelen,int valuetype)
{
	int buflen;
	unsigned char tag[3];
    _get_tag(name, namelen,tag);

	
	memcpy(buf+len,tag,3);
	if(valuetype == Value_Type_STR){
		buf[len+3] = (valuelen+1)/256 + valuetype;
		buf[len+4] = (valuelen+1)%256;	
	}
	else{
		buf[len+3] = valuelen/256 + valuetype;
		buf[len+4] = valuelen%256;		
	}
	memcpy(buf + len +2+3,value,valuelen);	
	
	if(valuetype == Value_Type_STR){
		buf[len+2+3+valuelen] = 0;
		buflen = len + 2 + 3 + 1 + valuelen;		
	}
	else{
		buflen = len + 2 + 3 + valuelen;
	}
	return buflen;
}

int pack_data_headtail(unsigned char serial,unsigned char *buf,int buflen)
{
	buf[0] = 0xaa;
	buf[1] = 0xbb;
	buf[2] = serial;
	buf[3] = (buflen+5)%256;
	buf[4] = (buflen+5)/256;
	buf[5] = buflen%256;
	buf[6] = buflen/256;	
	buf[buflen +7] = 0xcc;
	buf[buflen +8] = 0xdd;	 
	buf[buflen +9] = _get_lrc(buf+2,buflen+7);
	
	return buflen+10;
}

int check_data(unsigned char *buf,int len,unsigned char serialno)
{
	int i=0;
	int startpos = -1;
	int packagelen = -1;
	
	for(i=0;i<len-7;i++)
	{
		if(buf[i]== 0xaa && buf[i+1] == 0xbb){
			startpos = i;
			if(buf[i+2] != serialno ){
				return -2;
			}
			packagelen = buf[i+3]+buf[i+4]*256;
			if(packagelen + 5 > len)
				return -3;

			if(buf[i+packagelen+2] == 0xcc && buf[i+packagelen+3] == 0xdd ){
				unsigned char lrc = _get_lrc(buf + i + 2,packagelen + 2);
				unsigned char tail_lrc = buf[i+packagelen + 4];
				if(lrc == tail_lrc)
					return startpos;
			}
			
			break;
		}
	}
	return -4;
	
}

int getdata(unsigned char *buf,int len,int startpos,
	unsigned char *name,int namlen,unsigned char *value)
{
	unsigned char tag[3];
	int i = 0;
	int tlvlen = 0;
	
	_get_tag(name,namlen,tag);
	tlvlen = buf[5+startpos] + buf[6+startpos];

	for(i = startpos + 7;i<startpos + 7 + tlvlen -3;i++){
		if(buf[i] == tag[0] && buf[i+1] == tag[1] && buf[i+2] == tag[2]){
			int valuelen = (buf[i+3]&0x0f)*256 + buf[i+4];
			memcpy(value,buf+i+5,valuelen);
			return valuelen;
		}
	}
	
	return -1;
}

