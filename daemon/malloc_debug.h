#ifndef __MALLOC_DEBUG__
#define __MALLOC_DEBUG__

#include <stdint.h>

#ifdef MALLOC_DEBUG_LEVEL

void print_malloc_list(char *file_name, int only_count);

#define malloc(size) malloc_call_d( __LINE__ , __FILE__, __FUNCTION__, size)
#define calloc(num, size) calloc_call_d( __LINE__ , __FILE__, __FUNCTION__, num, size)
#define free(addr) free_call_d(__LINE__, __FILE__, __func__, addr)

void *malloc_call_d(int line, const char *file_name, const char *func_name,
		    size_t size);

void *calloc_call_d(int line, const char *file_name, const char *func_name,
		    size_t n, size_t size);

void free_call_d(int line, const char *file_name, const char *function,
		 void *addr);
#else /* MALLOC_DEBUG_LEVEL */

#define print_malloc_list(...) do{}while(0)

#endif /* MALLOC_DEBUG_LEVEL */

#endif /* __MALLOC_DEBUG__ */
