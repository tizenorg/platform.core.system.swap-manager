
#include <stdint.h>
#include <sys/types.h>

#include "pthread.h"
#include "swap_debug.h"
#include "malloc_debug.h"

#ifdef MALLOC_DEBUG_LEVEL

#undef strdup
#undef calloc
#undef malloc
#undef free

#if MALLOC_DEBUG_LEVEL == 2
#define logi LOGI
#define loge LOGE
#else
#define logi(...)
#define loge(...)
#endif

#if MALLOC_DEBUG_BOUNDS_CHECK
#define MALLOC_BOUND 100
#else
#define MALLOC_BOUND 0
#endif

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

struct mlist_t {
	void *next;
	void *prev;
	void *addr;
	int line;
	char *info;
	uint32_t size;
};

struct mlist_t *files_list;

void lock()
{
	pthread_mutex_lock(&global_mutex);
}

void unlock()
{
	pthread_mutex_unlock(&global_mutex);
}

static int list_append(struct mlist_t **to, struct mlist_t *from)
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

static void list_rm_el(struct mlist_t **list, struct mlist_t *element)
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

static struct mlist_t *find_malloc(struct mlist_t *file_list, void *addr)
{
	struct mlist_t *el = file_list->addr;

	while (el != NULL) {
		if (el->addr == addr)
			return el;
		el = el->next;
	}

	return NULL;
}

static struct mlist_t *find_list(const char *name)
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

#define BOUND_GOOD (0)
#define BOUND_LEFT_BAD (1<<1)
#define BOUND_RIGHT_BAD (1<<2)

#ifdef MALLOC_DEBUG_BOUNDS_CHECK
#define MALLOC_BOUND_MAGIC ((char)0xA5)
static void __fill_bounds(void *addr, size_t size)
{
	size_t i;
	char *p_left = addr;
	char *p_right = addr + MALLOC_BOUND + size;
	for (i = 0; i < MALLOC_BOUND; i++) {
		*p_left = MALLOC_BOUND_MAGIC;
		*p_right = MALLOC_BOUND_MAGIC;
		p_left++;
		p_right++;
	}
}

static int __check_bounds(void *addr, size_t size)
{

	int res = BOUND_GOOD;
	size_t i;
	char *p_left = addr - MALLOC_BOUND;
	char *p_right = addr + size;
	for (i = 0; i < MALLOC_BOUND; i++) {
		if ((int)(*p_left) != (int)MALLOC_BOUND_MAGIC) {
			res |= BOUND_LEFT_BAD;
			LOGE("fail left %d 0x%02x\n", i, *p_left);
		}
		if ((int)(*p_right) != (int)MALLOC_BOUND_MAGIC) {
			res |= BOUND_RIGHT_BAD;
			LOGE("fail right %d 0x%02x\n", i, *p_right);
		}
		p_left++;
		p_right++;
	}
	return res;
}

char *__bound_to_str(int b)
{
	switch (b) {
		case BOUND_GOOD:
			return "+";
		case BOUND_RIGHT_BAD:
			return "BOUND_RIGHT_BAD";
		case BOUND_LEFT_BAD:
			return "BOUND_LEFT_BAD";
		case BOUND_LEFT_BAD | BOUND_RIGHT_BAD:
			return "BOUND_LEFT_BAD BOUND_RIGHT_BAD";
		default:
			return "WRONG BOUND CODE";
	}
}
#else

static void __fill_bounds(void *addr, size_t size)
{
}

static int __check_bounds(void *addr, size_t size)
{
	return BOUND_GOOD;
}

char *__bound_to_str(int b)
{
	return "disabled";
}

#endif

static __print_el(struct mlist_t *el)
{
#ifdef MALLOC_DEBUG_BOUNDS_CHECK
	int res = __check_bounds(el->addr, el->size);
	if (res == BOUND_GOOD) {
		LOGI("  %04d) 0x%lX <%s>\n", el->line, el->addr,
		     el->info);
	} else {
		LOGE("  %04d) 0x%lX <%s> <%s>\n", el->line, el->addr,
		    __bound_to_str(res),
		     el->info);
	}
#else
	LOGI("  %04d) 0x%lX <%s>\n", el->line, el->addr,
	     el->info);
#endif

}

void print_file_malloc_list(struct mlist_t *file, int only_count)
{
	struct mlist_t *el = NULL;
	int count = 0;

	LOGI(" -> malloc list for file (%s)\n", file->info);
	el = file->addr;
	if (el == NULL)
		LOGI("list is empty\n");
	while (el != NULL) {
		count++;

		if (only_count == 0)
			__print_el(el);
		el = el->next;
	}

	if (only_count == 1)
		LOGI("  malloc count = %d\n", count);
}

void print_malloc_list(char *file_name, int only_count)
{
	struct mlist_t *file = NULL;
	LOGI("BEGIN--------------------------------------------------------\n");
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
	LOGI("END----------------------------------------------------------\n");
}

static size_t rm_malloc(struct mlist_t *file_list, void *addr, int *err)
{
	int code;
	struct mlist_t *el = find_malloc(file_list, addr);
	*err = BOUND_GOOD;

	if (el != NULL) {
#if MALLOC_DEBUG_BOUNDS_CHECK
		code = __check_bounds(el->addr, el->size);
		if (code != BOUND_GOOD) {
			__print_el(el);
			*err = code;
		}
#endif
		list_rm_el(&(file_list->addr), el);
		return 1;
	} else {
		return 0;
	}
}

static void add_malloc(struct mlist_t **list, void *addr, int line,
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
	lock();
	size_t size = strlen(str) + 1;
	void *addr = strdup(str);

	if (MALLOC_BOUND != 0) {
		void *tmp_addr = malloc(size + MALLOC_BOUND * 2);
		memcpy(tmp_addr + MALLOC_BOUND, addr, size);
		__fill_bounds(tmp_addr, size);
		free(addr);
		addr = tmp_addr + MALLOC_BOUND;
	}

	logi("strdup> 0x%0lX (%d:%s '%s')\n", addr, line, func_name, file_name);
	struct mlist_t *file_el = find_list(file_name);

	add_malloc(&(file_el->addr), addr, line, file_name, func_name, strlen(str) + 1);

	unlock();
	return addr;
}

void *malloc_call_d(int line, const char *file_name, const char *func_name,
		    size_t size)
{
	logi("malloc>\n");
	lock();
	void *addr = malloc(size + MALLOC_BOUND * 2);
	__fill_bounds(addr, size);
	addr += MALLOC_BOUND;
	logi("malloc> 0x%0lX (%d:%s '%s')\n", addr, line, func_name, file_name);
	struct mlist_t *file_el = find_list(file_name);

	add_malloc(&(file_el->addr), addr, line, file_name, func_name, size);

	unlock();
	return addr;
}

void *calloc_call_d(int line, const char *file_name, const char *func_name,
		    size_t n, size_t size)
{

	logi("calloc>\n");
	lock();
	void *addr = malloc(n * size + MALLOC_BOUND * 2);
	    //calloc(n, size + MALLOC_BOUND * 2);
	__fill_bounds(addr, size);
	addr += MALLOC_BOUND;
	memset(addr, 0, n * size);

	logi("calloc> 0x%0lX (%d:%s '%s')\n", addr, line, func_name, file_name);
	struct mlist_t *file_el = find_list(file_name);

	add_malloc(&(file_el->addr), addr, line, file_name, func_name,
		   size * n);

	unlock();
	return addr;
}

void free_call_d(int line, const char *file_name, const char *function,
		 void *addr)
{
	struct mlist_t *list = NULL;
	size_t size = 0;
	int err = BOUND_GOOD;
	lock();
	list = find_list(file_name);
	if (!rm_malloc(list, addr, &err)) {
		list = files_list;
		while (list != NULL) {
			if (rm_malloc(list, addr, &err)) {
				free(addr - MALLOC_BOUND);
				goto exit;
			}
			list = list->next;
		}

	} else {
		logi("free addr 0x%08lX (%d:%s '%s')\n",
		     addr, line, function, file_name);
		free(addr - MALLOC_BOUND);
		goto exit;
	}
	LOGW("cannot free element!!! 0x%08lX (%d:%s '%s')\n",
	     addr, line, function, file_name);

exit:

#if MALLOC_DEBUG_BOUNDS_CHECK
	if (err != BOUND_GOOD) {
	LOGW("%s 0x%08lX (%d:%s '%s')\n", __bound_to_str(err),
	     addr, line, function, file_name);
	}
#endif

	unlock();

}

//redefine functions
#define strdup(str) malloc_call_d( __LINE__ , __FILE__, __FUNCTION__, str)
#define malloc(size) malloc_call_d( __LINE__ , __FILE__, __FUNCTION__, size)
#define calloc(num, size) calloc_call_d( __LINE__ , __FILE__, __FUNCTION__, num, size)
#define free(addr) free_call_d(__LINE__, __FILE__, __func__, addr)

#else

#endif /* MALLOC_DEBUG_LEVEL */
