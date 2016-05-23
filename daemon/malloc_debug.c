
#include <stdint.h>
#include <sys/types.h>

#include "swap_debug.h"
#include "malloc_debug.h"

#ifdef MALLOC_DEBUG_LEVEL

#undef strdup
#undef calloc
#undef malloc
#undef free

#if MALLOC_DEBUG_LEVEL == 2
#define logi SWAP_LOGI
#define loge SWAP_LOGE
#else
#define logi(...)
#define loge(...)
#endif

struct mlist_t {
	void *next;
	void *prev;
	void *addr;
	int line;
	char *info;
	uint32_t size;
};

struct mlist_t *files_list;

int list_append(struct mlist_t **to, struct mlist_t *from)
{
	struct mlist_t *p = NULL;

	if (*to == NULL) {
		//empty list
		*to = from;
	} else {
		p = *to;
		*to = from;
		from->next = (void *)p;
		p->prev = (void *)from;
	}

	return 0;
}

void list_rm_el(struct mlist_t **list, struct mlist_t *element)
{
	struct mlist_t *prev = element->prev;
	struct mlist_t *next = element->next;

	if (element != NULL) {
		if (prev != NULL)
			//prev != null, next == null
			//prev != null, next != null
			prev->next = next;
		else
			//prev == null, next == null
			//prev == null, next != null
			*list = next;

		if (next != NULL)
			next->prev = prev;
	}

	free(element);
}

struct mlist_t *find_malloc(struct mlist_t *file_list, void *addr)
{
	struct mlist_t *el = file_list->addr;

	while (el != NULL) {
		if (el->addr == addr)
			return el;
		el = el->next;
	}

	return NULL;
}

struct mlist_t *find_list(const char *name)
{
	struct mlist_t *el = files_list;

	while (el != NULL) {
		if (strcmp(el->info, name) == 0)
			return el;
		el = el->next;
	}
	el = malloc(sizeof(struct mlist_t));
	memset(el, 0, sizeof(struct mlist_t));
	el->info = name;
	list_append(&files_list, el);

	return el;
}

void print_file_malloc_list(struct mlist_t *file, int only_count)
{
	struct mlist_t *el = NULL;
	int count = 0;

	SWAP_LOGI(" -> malloc list for file (%s)\n", file->info);
	el = file->addr;
	if (el == NULL)
		SWAP_LOGI("list is empty\n");
	while (el != NULL) {
		count++;
		if (only_count == 0)
			SWAP_LOGI("  %04d) 0x%lX <%s>\n", el->line, el->addr,
			     el->info);
		el = el->next;
	}

	if (only_count == 1)
		SWAP_LOGI("  malloc count = %d\n", count);
}

void print_malloc_list(char *file_name, int only_count)
{
	struct mlist_t *file = NULL;
	SWAP_LOGI("BEGIN--------------------------------------------------------\n");
	if (file_name == NULL) {
		file = files_list;
		while (file != NULL) {
			print_file_malloc_list(file, only_count);
			file = file->next;
		}
	} else {
		struct mlist_t *file = find_list(file_name);
		print_file_malloc_list(file, only_count);
	}
	SWAP_LOGI("END----------------------------------------------------------\n");
}

int rm_malloc(struct mlist_t *file_list, void *addr)
{
	struct mlist_t *el = find_malloc(file_list, addr);

	if (el != NULL) {
		list_rm_el(&(file_list->addr), el);
		return 1;
	} else {
		return 0;
	}
}

void add_malloc(struct mlist_t **list, void *addr, int line,
		const char *file_name, const char *func_name, size_t size)
{
	struct mlist_t *el = malloc(sizeof(*el));

	el->addr = addr;
	el->line = line;
	el->info = func_name;
	el->size = size;
	el->next = NULL;
	el->prev = NULL;

	list_append(list, el);

}

char *strdub_call_d(int line, const char *file_name, const char *func_name,
		    char *str)
{
	logi("strdup>\n");
	void *addr = strdup(str);
	logi("strdup> 0x%0lX (%d:%s '%s')\n", addr, line, func_name, file_name);
	struct mlist_t *file_el = find_list(file_name);

	add_malloc(&(file_el->addr), addr, line, file_name, func_name, strlen(str) + 1);

	return addr;
}

void *malloc_call_d(int line, const char *file_name, const char *func_name,
		    size_t size)
{
	logi("malloc>\n");
	void *addr = malloc(size);
	logi("malloc> 0x%0lX (%d:%s '%s')\n", addr, line, func_name, file_name);
	struct mlist_t *file_el = find_list(file_name);

	add_malloc(&(file_el->addr), addr, line, file_name, func_name, size);

	return addr;
}

void *calloc_call_d(int line, const char *file_name, const char *func_name,
		    size_t n, size_t size)
{

	logi("calloc>\n");
	void *addr = calloc(n, size);
	logi("calloc> 0x%0lX (%d:%s '%s')\n", addr, line, func_name, file_name);
	struct mlist_t *file_el = find_list(file_name);

	add_malloc(&(file_el->addr), addr, line, file_name, func_name,
		   size * n);

	return addr;
}

void free_call_d(int line, const char *file_name, const char *function,
		 void *addr)
{

	struct mlist_t *list = find_list(file_name);
	if (!rm_malloc(list, addr)) {
		list = files_list;
		while (list != NULL) {
			if (rm_malloc(list, addr)) {
				free(addr);
				return;
			}
			list = list->next;
		}

	} else {
		logi("free addr 0x%08lX (%d:%s '%s')\n",
		     addr, line, function, file_name);
		free(addr);
		return;
	}
	SWAP_LOGW("cannot free element!!! 0x%08lX (%d:%s '%s')\n",
	     addr, line, function, file_name);

}

//redefine functions
#define strdup(str) malloc_call_d( __LINE__ , __FILE__, __FUNCTION__, str)
#define malloc(size) malloc_call_d( __LINE__ , __FILE__, __FUNCTION__, size)
#define calloc(num, size) calloc_call_d( __LINE__ , __FILE__, __FUNCTION__, num, size)
#define free(addr) free_call_d(__LINE__, __FILE__, __func__, addr)

#else

#endif /* MALLOC_DEBUG_LEVEL */
