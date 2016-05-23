/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
 * Nikita Kalyazin    <n.kalyazin@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Contributors:
 * - Samsung RnD Institute Russia
 *
 */


#include <ctype.h>

#include "swap_debug.h"

#ifdef DEB_PRINTBUF
//TODO del it or move to debug section
void print_buf(char * buf, int len, const char *info)
{
	int i,j;
	char local_buf[3*17 + 2*16 + 1 + 8];
	char * p1, * p2;

	SWAP_LOGI("BUFFER [%d] <%s>:\n", len, info);
	for ( i = 0; i < len/16 + 1; i++)
	{
		memset(local_buf, ' ', 5*16 + 8);

		sprintf(local_buf, "0x%04X: ", i);
		p1 = local_buf + 8;
		p2 = local_buf + 8 + 3*17;
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
		SWAP_LOGI("%s\n",local_buf);
	}
}
#else
inline void print_buf(char * buf, int len, const char *info) {
	return;
}
#endif
