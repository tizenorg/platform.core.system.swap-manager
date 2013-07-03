
#include <ctype.h>

#include "debug.h"

#ifdef DEB_PRINTBUF
//TODO del it or move to debug section
void printBuf (char * buf, int len)
{
	int i,j;
	char local_buf[3*16 + 2*16 + 1];
	char * p1, * p2;

	LOGI("BUFFER:\n");
	for ( i = 0; i < len/16 + 1; i++)
	{
		memset(local_buf, ' ', 5*16);
		p1 = local_buf;
		p2 = local_buf + 3*17;
		for ( j = 0; j < 16; j++)
			if (i*16+j < len )
			{
				sprintf(p1, "%02X ",(unsigned char) *buf);
				p1+=3;
				if (isprint( *buf)){
					sprintf(p2, "%c ",(int)*buf);
				}else{
					sprintf(p2,". ");
				}
				p2+=2;
				buf++;
			}
		*p1 = ' ';
		*p2 = '\0';
		LOGI("%s\n",local_buf);
	}
}
#endif
