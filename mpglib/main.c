
#include "mpg123.h"
#include "mpglib.h"

char buf[16384];
struct mpstr mp;

void main(void)
{
	int size;
	char out[8192];
	int len,ret;
	

	InitMP3(&mp);

	while(1) {
		len = read(0,buf,16384);
		if(len <= 0)
			break;
		ret = decodeMP3(&mp,buf,len,out,8192,&size);
		while(ret == MP3_OK) {
			write(1,out,size);
			ret = decodeMP3(&mp,NULL,0,out,8192,&size);
		}
	}

}

