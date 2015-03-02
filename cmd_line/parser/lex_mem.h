#ifndef __LEX_MEM_H__
#define __LEX_MEM_H__

#define COPY_LEX_STR(from_str, to_str)				\
  to_str = (char *)malloc( strlen(from_str) + 1 );	\
	strncpy(to_str, from_str + 1, strlen(from_str) + 1);

#define DELETE_LEX_STR(str)	\
	free(str)


#endif
