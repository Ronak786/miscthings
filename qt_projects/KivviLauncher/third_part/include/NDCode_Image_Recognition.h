#ifndef NANOID_RUN_
#define NANOID_RUN_

extern "C" {

int nanoid_hexagon_recognition(unsigned char* imdata, int img_rows, int img_cols, int channels,
	char* param, int* bitsArray, int bitArray_len,
	int* boundingbox, int bdx_len);
	
}

#endif
