#ifndef __MALLOC_DEBUG__
#define __MALLOC_DEBUG__

#include <stdint.h>

#ifdef MALLOC_DEBUG_ON
struct mlist_t{
	void *next;
	void *prev;
	void *addr;
	int  line;
	char *info;
	uint32_t size;
};

void print_malloc_list(char *file_name, int only_count);

#define malloc(size) malloc_call_d( __LINE__ , __FILE__, __FUNCTION__, size)
#define calloc(num, size) calloc_call_d( __LINE__ , __FILE__, __FUNCTION__, num, size)
#define free(addr) free_call_d(__LINE__, __FILE__, __func__, addr)

#else /* MALLOC_DEBUG_ON */

#define print_malloc_list(...) do{}while(0)

#endif /* MALLOC_DEBUG_ON */

#endif /* __MALLOC_DEBUG__ */
