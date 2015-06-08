/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
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

// TODO check memory (malloc, free)

#include <stdint.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "da_inst.h"
#include "da_protocol.h"
#include "da_protocol_inst.h"
#include "debug.h"
#include "daemon.h"

struct lib_list_t *new_lib_inst_list = NULL;

uint32_t app_count = 0;
char *packed_app_list = NULL;
char *packed_lib_list = NULL;

static struct _msg_target_t *lib_maps_message = NULL;
static pthread_mutex_t lib_inst_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lib_maps_message_mutex = PTHREAD_MUTEX_INITIALIZER;

uint32_t libs_count;

//----------------------------------- lists ----------------------------------

static int data_list_add_probe_to_hash(struct data_list_t *to, struct probe_list_t *probe)
{
	// TODO add hash
	return 0;
}

// create hash for lib
static int data_list_make_hash(struct data_list_t *what)
{
	struct probe_list_t *p;

	for (p = what->list; p != NULL; p = p->next)
		data_list_add_probe_to_hash(what, p);
	return 0;
}

//free memory
static void free_probe_element(struct probe_list_t *probe)
{
	free(probe->func);
	free(probe);
}

static void free_data_element(struct data_list_t *lib)
{
	free(lib->data);
	free(lib);
}

static void free_probe_list(struct probe_list_t *probe)
{
	struct probe_list_t *next;
	while (probe != NULL) {
		next = probe->next;
		free_probe_element(probe);
		probe = next;
	}
}

static void free_data(struct data_list_t *lib)
{
	free_probe_list(lib->list);
	free_data_element(lib);
}

static struct data_list_t *new_data(void)
{
	struct data_list_t *lib = malloc(sizeof(*lib));

	if (lib) {
		lib->next = NULL;
		lib->prev = NULL;
		lib->hash = 0;
		lib->size = 0;
		lib->list = NULL;
	} else {
		LOGI("Cannot allocate memory for struct data_list_t\n");
	}

	return lib;
}

struct lib_list_t *new_lib(void)
{
	struct lib_list_t *lib = NULL;

	lib = (struct lib_list_t *)new_data();
	if (lib == NULL) {
		LOGE("cannot create lib\n");
		goto exit_fail;
	}

	lib->lib = malloc(sizeof(*lib->lib));
	if (lib->lib == NULL) {
		LOGE("can not malloc buffer for probe_list_t lib\n");
		goto exit_fail_free_lib;
	}

	memset(lib->lib, 0, sizeof(*lib->lib));

	/* SUCCESS */
	return lib;

exit_fail_free_lib:
	free_data(lib);
exit_fail:
	return NULL;
}

struct app_list_t *new_app(void)
{
	struct app_list_t *app = NULL;

	app = (struct app_list_t *)new_data();
	if (app == NULL) {
		LOGE("cannot create app\n");
		goto exit_fail;
	}

	app->app = malloc(sizeof(*app->app));
	if (app->app == NULL) {
		LOGE("can not malloc buffer for app_info_t app\n");
		goto exit_fail_free_app;
	}

	memset(app->app, 0, sizeof(*app->app));

	/* SUCCESS */
	return app;

exit_fail_free_app:
	free(app);
exit_fail:
	return NULL;
}

struct probe_list_t *new_probe(void)
{
	struct probe_list_t *probe;
	probe = malloc(sizeof(*probe));
	if (probe != NULL) {
		probe->next = NULL;
		probe->prev = NULL;
		probe->size = 0;
		probe->func = NULL;
	}
	return probe;
}

void free_data_list(struct data_list_t **data)
{
	struct data_list_t *next;
	while (*data != NULL) {
		next = (*data)->next;
		free_data(*data);
		*data = next;
	}
}

//------------ create - destroy


//------------- add - remove
int data_list_append(struct data_list_t **to, struct data_list_t *from)
{
	struct data_list_t *p = NULL;
	if (*to == NULL) {
		// empty list
		*to = from;
	} else {
		p = *to;
		*to = from;
		from->next = (void *)p;
		p->prev = (void *)from;
	}
	return 0;
}


static int data_list_append_probes_hash(struct data_list_t *to, struct data_list_t *from)
{
	struct probe_list_t *p = from->list;
	struct probe_list_t *last = p;

	to->size += p->size;
	to->func_num += from->func_num;
	for (p = from->list; p != NULL; p = p->next) {
		data_list_add_probe_to_hash(to, p);
		last = p;
	}

	last->next = to->list;
	to->list->prev = last;
	to->list = from->list;

	return 1;
}

int probe_list_append(struct data_list_t *to, struct probe_list_t *from)
{
	struct probe_list_t **list = &(to->list);
	struct probe_list_t *p = NULL;
	uint32_t num = 0;
	if (*list == NULL) {
		// empty list
		*list = from;
	} else {
		p = *list;
		*list = from;
		from->next = (void *)p;
		p->prev = (void *)from;
	}
	to->size += from->size;

	num = 0;
	for (p = from; p != NULL; p = p->next)
		num++;
	to->func_num += num;
	return 0;
}

static struct probe_list_t *probe_list_rm_element(struct data_list_t *list, struct probe_list_t *element)
{
	struct probe_list_t *prev = element->prev;
	struct probe_list_t *next = element->next;

	if (prev != NULL)
		prev->next = next;
	else
		list->list = next;

	if (next != NULL)
		next->prev = prev;

	list->size -= element->size;

	list->func_num--;
	free_probe_element(element);
	return next;
}

static struct data_list_t *data_list_unlink_data(struct data_list_t **list, struct data_list_t *element)
{
	struct data_list_t *prev = element->prev;
	struct data_list_t *next = element->next;

	if (prev != NULL)
		/* prev != null, next == null */
		/* prev != null, next != null */
		prev->next = next;
	else
		/* prev == null, next == null */
		/* prev == null, next != null */
		*list = next;

	if (next != NULL)
		next->prev = (struct lib_list_t *)prev;

	element->prev = NULL;
	element->next = NULL;

	return next;
}

static struct data_list_t *data_list_rm_data(struct data_list_t **list, struct data_list_t *element)
{
	struct data_list_t *next = NULL;

	next = data_list_unlink_data(list, element);
	free_data_element(element);

	return next;
}

// find
static struct data_list_t *data_list_find_data(struct data_list_t *whered, struct data_list_t *whatd, cmp_data_f cmp)
{
	struct data_list_t *where;
	struct data_list_t *what = whatd;
	for (where = whered; where != NULL; where = where->next) {
		if (where->hash == what->hash) {
			if (cmp(where, what))
				return where;
		}
	}
	return NULL;
}

static struct probe_list_t *find_probe(struct data_list_t *where, struct probe_list_t *probe)
{
	struct probe_list_t *p ;
	for (p = where->list; p != NULL; p = p->next)
		if (p->size == probe->size)
			if (p->func->func_addr == probe->func->func_addr)
				if (strcmp(p->func->args, probe->func->args) == 0)
					break;

	return p;
}

// "from" will be destroyed after this call
static int data_list_move_with_hash(struct data_list_t **to, struct data_list_t **from, cmp_data_f cmp)
{

	struct data_list_t *p = *from;
	struct data_list_t *next = NULL;
	struct data_list_t *sch = NULL;
	while (p != NULL) {
		sch = data_list_find_data(*to, p, cmp);
		next = data_list_unlink_data(from, p);
		if (sch == NULL) {
			data_list_make_hash(p);
			data_list_append(to, p);
		} else {
			data_list_append_probes_hash(sch, p);
		}
		p = next;
	}
	return 1;
}

//---------------------------- collisions resolve ------------------------------

static int cmp_libs(struct data_list_t *el_1, struct data_list_t *el_2)
{
	return (strcmp(
			((struct lib_list_t *)el_1)->lib->bin_path,
			((struct lib_list_t *)el_2)->lib->bin_path
			) == 0);
}

///////////////////////////////////////////////////////////////////////////
// function removes from new list all probes which are already installed
//
// cur - current state (lib list with probes)
// new - additional probes which were received with MSG_SWAP_INST_ADD msg
//       (lib list with probes)
// this function removes all probes which are present in cur and new list from
// new list so after this function call in new list will be only probes which are
// not present in cur list
static int resolve_collisions_for_add_msg(struct lib_list_t **cur, struct lib_list_t **new)
{
	struct data_list_t *p = (struct data_list_t *)*new;
	struct data_list_t *sch = NULL;

	struct probe_list_t *pr = NULL;
	struct probe_list_t *next_pr = NULL;

	// remove collisions from list "new"
	while (p != NULL) {
		sch = data_list_find_data((struct data_list_t *)*cur, p, cmp_libs);
		if (sch == NULL) {
			// lib not found in cur config
		} else {
			// lib found in cur config so resolve collisions
			pr = p->list;
			while (pr != NULL) {
				// remove collisions
				if (find_probe(sch, pr) != NULL) {
					// probe already exist
					// rm from new config
					next_pr = probe_list_rm_element(p, pr);
					pr = next_pr;
				} else {
					pr = pr->next;
				}
			}

			// rm lib if it is empty
			if (p->list == NULL) {
				p = data_list_rm_data((struct data_list_t **)new, p);
				continue;
			}
		}
		p = p->next;
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////
// function removes from cur list all probes which are in list "new"
//         and removes from "new" all probes which are not instaled
//
// cur - current state (lib list with probes)
// new - list for probes remove which were received with MSG_SWAP_INST_REMOVE msg
//
// this function removes all probes which are present in cur and new list from
// cur list and removes all probes which are present in new but not present in
// cur list so after this function call in new list will be only probes which are
// present in cur list
static int resolve_collisions_for_rm_msg(struct lib_list_t **cur, struct lib_list_t **new)
{
	struct data_list_t *p = (struct data_list_t *)*new;
	struct data_list_t *next_l = NULL;
	struct data_list_t *sch = NULL;

	struct probe_list_t *pr = NULL;
	struct probe_list_t *next_pr = NULL;
	struct probe_list_t *tmp = NULL;

	// remove collisions from list "new"
	while (p != NULL) {
		sch = data_list_find_data((struct data_list_t *)*cur, p, cmp_libs);
		if (sch == NULL) {
			//lib not found so no probes remove
			next_l = data_list_rm_data((struct data_list_t **)new, p);
		} else {
			//lib found so we need remove collisions
			pr = p->list;
			if (pr == NULL) {
				// if rm probe list is empty that is mean we need
				// to remove all probes on this lib
				data_list_unlink_data((struct data_list_t **)cur, sch);
				data_list_append((struct data_list_t **)new, sch);
				next_l = data_list_rm_data((struct data_list_t **)cur, p);
			} else {
				// lib is not empty so merge
				while (pr != NULL) {
					// remove collisions
					if ((tmp = find_probe(sch, pr)) != NULL) {
						// probe found so remove probe
						// from cur state
						probe_list_rm_element(sch, tmp);
						pr = pr->next;
					} else {
						// probe no found so remove it
						// from new state
						next_pr = probe_list_rm_element(p, pr);
						pr = next_pr;
					}
				}
				// rm lib if it is empty
				if (sch->list == NULL) {
					data_list_rm_data((struct data_list_t **)cur, sch);
				}
				next_l = p->next;
			}
		}
		p = next_l;
	}
	return 1;
}

//--------------------------------------pack ----------------------------------

static char *pack_lib_head_to_array(char *to, void *data)
{
	struct us_lib_inst_t *lib = data;
	pack_str(to, lib->bin_path);
	return to;
}

static char *pack_app_head_to_array(char *to, void *data)
{
	struct app_info_t *app = data;
	pack_int32(to, app->app_type);
	pack_str(to, app->app_id);
	pack_str(to, app->exe_path);
	return to;
}

static char *pack_data_to_array(struct data_list_t *data, char *to, pack_head_t pack_head)
{
	struct probe_list_t *p;

	to = pack_head(to, data->data);
	pack_int32(to, data->func_num);
	for (p = data->list; p != NULL; p = p->next) {
		memcpy(to, p->func, p->size);
		to += p->size;
	}
	return to;
}

static char *pack_data_list_to_array(struct data_list_t *list, uint32_t *len, uint32_t *count, pack_head_t pack)
{
	char *res = NULL;
	char *to = NULL;
	uint32_t size = 0;
	uint32_t cnt = 0;
	struct data_list_t *p = list;

	for (p = list; p != NULL; p = p->next) {
		size += p->size;
		cnt++;
	}

	size += sizeof(uint32_t);
	*len = size;
	*count = cnt;

	if (size != 0) {
		res = malloc(size);
		to = res;
		if (to != NULL) {
			pack_int32(to, cnt);
			for (p = list; p != NULL; p = p->next)
				to = pack_data_to_array(p, to, pack);
		} else {
			LOGE("can not malloc buffer for data list packing\n");
		}
	}
	return res;
}

static char *pack_lib_list_to_array(struct lib_list_t *list, uint32_t *size, uint32_t *count)
{
	return pack_data_list_to_array((struct data_list_t *)list, size,
					count, pack_lib_head_to_array);
}

static char *pack_app_list_to_array(struct app_list_t *list, uint32_t *size, uint32_t *count)
{
	return pack_data_list_to_array((struct data_list_t *)list, size,
					count,  pack_app_head_to_array);
}

static int generate_msg(struct msg_t **msg, struct lib_list_t *lib_list, struct app_list_t *app_list)
{
	uint32_t i,
		 size = 0,
		 libs_size = 0,
		 apps_size = 0,
		 libs_count = 0,
		 apps_count = 0;
		 int res = 1; /* no error */
	char	 *p = NULL;

	packed_lib_list = pack_lib_list_to_array(lib_list, &libs_size, &libs_count);
	// print_buf(packed_lib_list, libs_size, "LIBS");

	packed_app_list = pack_app_list_to_array(app_list, &apps_size, &apps_count);
	// print_buf(packed_app_list, apps_size, "APPS");

	size = apps_count * libs_size + apps_size;

	LOGI("size = %d, apps= %d, libs = %d\n", size, apps_count, libs_count);

	// add header size
	*msg = malloc(size + sizeof(**msg));
	if (*msg == NULL) {
		LOGI("Cannot allocate memory for struct msg_t \n");
		res = 0;
		goto exit_free;
	}

	memset(*msg, '*', size);

	p = (char *)*msg;
	pack_int32(p, 0);		// msg id
	pack_int32(p, size);		// payload size
	pack_int32(p, apps_count);

	struct app_list_t *app = app_list;
	char *app_p = packed_app_list + sizeof(((struct user_space_inst_t *)0)->app_num);

	for (i = 0; i < apps_count; i++) {
		memcpy(p, app_p, app->size);
		p += app->size;
		memcpy(p, packed_lib_list, libs_size);
		p += libs_size;

		app_p += app->size;
		app = app->next;
	}
exit_free:
	free(packed_lib_list);
	free(packed_app_list);

	// print_buf((char *)*msg, size, "ANSWER");
	return res;
}

static void lock_lib_maps_message()
{
	pthread_mutex_lock(&lib_inst_list_mutex);
}

static void unlock_lib_maps_message()
{
	pthread_mutex_unlock(&lib_inst_list_mutex);
}

static void lock_lib_list()
{
	pthread_mutex_lock(&lib_maps_message_mutex);
}

static void unlock_lib_list()
{
	pthread_mutex_unlock(&lib_maps_message_mutex);
}

static void generate_maps_inst_msg(struct user_space_inst_t *us_inst)
{
	lock_lib_maps_message();

	struct lib_list_t *lib = us_inst->lib_inst_list;
	struct app_list_t *app = us_inst->app_inst_list;
	char *p, *resolved;
	uint32_t total_maps_count = 0;
	uint32_t total_len = sizeof(total_maps_count) + sizeof(*lib_maps_message);
	char real_path_buf[PATH_MAX];

	/* total message len calculate */
	while (lib != NULL) {
		p = lib->lib->bin_path;
		total_len += strlen(p) + 1;
		total_maps_count++;
		LOGI("lib #%u <%s>\n", total_maps_count, p);
		lib = (struct lib_list_t *)lib->next;
	}

	while (app != NULL) {
		p = app->app->exe_path;
		resolved = realpath((const char *)p, real_path_buf);
		if (resolved != NULL) {
			total_len += strlen(real_path_buf) + 1;
			total_maps_count++;
			LOGI("app #%u <%s>\n", total_maps_count, resolved);
		} else {
			LOGE("cannot resolve bin path <%s>\n", p);
		}

		app = (struct app_list_t *)app->next;
	}

	if (lib_maps_message != NULL)
		free(lib_maps_message);

	lib_maps_message = malloc(total_len);
	lib_maps_message->type = MSG_MAPS_INST_LIST;
	lib_maps_message->length = total_len;

	/* pack data */
	p = lib_maps_message->data;
	pack_int32(p, total_maps_count);

	lib = us_inst->lib_inst_list;
	while (lib != NULL) {
		pack_str(p, lib->lib->bin_path);
		lib = (struct lib_list_t *)lib->next;
	}

	app = us_inst->app_inst_list;
	while (app != NULL) {
		resolved = realpath(app->app->exe_path, real_path_buf);
		if (resolved != NULL)
			pack_str(p, real_path_buf);
		app = (struct app_list_t *)app->next;
	}

	LOGI("total_len = %u\n", total_len);
	print_buf((char *)lib_maps_message, total_len, "lib_maps_message");

	unlock_lib_maps_message();
}

void send_maps_inst_msg_to(struct target *t)
{
	lock_lib_maps_message();
	target_send_msg(t, (struct msg_target_t *)lib_maps_message);
	unlock_lib_maps_message();
}

static void send_maps_inst_msg_to_all_targets()
{
	lock_lib_maps_message();
	target_send_msg_to_all(lib_maps_message);
	unlock_lib_maps_message();
}

//-----------------------------------------------------------------------------
struct app_info_t *app_info_get_first(struct app_list_t **app_list)
{
	*app_list = prof_session.user_space_inst.app_inst_list;
	if (*app_list == NULL) {
		return NULL;
	}

	return (*app_list)->app;
}

struct app_info_t *app_info_get_next(struct app_list_t **app_list)
{
	if (*app_list == NULL)
		return NULL;

	if ((*app_list = (*app_list)->next) == NULL)
		return NULL;

	return (*app_list)->app;
}

//-----------------------------------------------------------------------------
int msg_start(struct msg_buf_t *data, struct user_space_inst_t *us_inst,
	      struct msg_t **msg, enum ErrorCode *err)
{
	int res = 0;
	char *p = NULL;
	*msg = NULL;

	/* lock list access */
	lock_lib_list();

	if (!parse_app_inst_list(data, &us_inst->app_num, &us_inst->app_inst_list)) {
		*err = ERR_WRONG_MESSAGE_FORMAT;
		LOGE("parse app inst\n");
		res = 1;
		goto msg_start_exit;
	}

	generate_msg(msg, us_inst->lib_inst_list, us_inst->app_inst_list);

	if (*msg != NULL) {
		p = (char *)*msg;
		pack_int32(p, NMSG_START);
	} else {
		*err = ERR_CANNOT_START_PROFILING;
		res = 1;
		goto msg_start_exit;
	}

	generate_maps_inst_msg(us_inst);

msg_start_exit:
	/* unlock list access */
	unlock_lib_list();

	return res;
}

int msg_swap_inst_add(struct msg_buf_t *data, struct user_space_inst_t *us_inst,
		      struct msg_t **msg, enum ErrorCode *err)
{
	int res = 0;
	uint32_t lib_num = 0;
	char *p = NULL;

	*err = ERR_UNKNOWN;

	/* lock list access */
	lock_lib_list();

	if (!parse_lib_inst_list(data, &lib_num, &us_inst->lib_inst_list)) {
		*err = ERR_WRONG_MESSAGE_FORMAT;
		LOGE("parse lib inst list fail\n");
		res = 1;
		goto msg_swap_inst_add_exit;
	}
	// rm probes from new if its presents in cur
	if (!resolve_collisions_for_add_msg(&us_inst->lib_inst_list, &new_lib_inst_list)) {
		LOGE("resolve collision\n");
		res = 1;
		goto msg_swap_inst_add_exit;
	};

	// generate msg to send
	if (us_inst->app_inst_list != NULL) {
		generate_msg(msg, new_lib_inst_list, us_inst->app_inst_list);
		p = (char *)*msg;
		pack_int32(p, NMSG_SWAP_INST_ADD);
	}
	// apply changes to cur state
	if (!data_list_move_with_hash(
		(struct data_list_t **)&us_inst->lib_inst_list,
		(struct data_list_t **)&new_lib_inst_list,
		cmp_libs))
	{
		LOGE("data move\n");
		res = 1;
		goto msg_swap_inst_add_exit;
	};

	// free new_list
	free_data_list((struct data_list_t **)&new_lib_inst_list);
	new_lib_inst_list = NULL;
	*err = ERR_NO;

	generate_maps_inst_msg(us_inst);
	send_maps_inst_msg_to_all_targets();

msg_swap_inst_add_exit:
	/* unlock list access */
	unlock_lib_list();

	return res;
}

int msg_swap_inst_remove(struct msg_buf_t *data, struct user_space_inst_t *us_inst,
			 struct msg_t **msg, enum ErrorCode *err)
{
	int res = 0;
	uint32_t lib_num = 0;
	char *p = NULL;
	*err = ERR_UNKNOWN;

	/* lock list access */
	lock_lib_list();

	if (!parse_lib_inst_list(data, &lib_num, &new_lib_inst_list)) {
		*err = ERR_WRONG_MESSAGE_FORMAT;
		LOGE("parse lib inst\n");
		res = 1;
		goto msg_swap_inst_remove_exit;
	}

	if (!resolve_collisions_for_rm_msg(&us_inst->lib_inst_list, &new_lib_inst_list)) {
		LOGE("resolve collisions\n");
		res = 1;
		goto msg_swap_inst_remove_exit;
	}

	if (us_inst->app_inst_list != NULL) {
		if (!generate_msg(msg, new_lib_inst_list, us_inst->app_inst_list)) {
			LOGE("generate msg\n");
			res = 1;
			goto msg_swap_inst_remove_exit;
		}
		p = (char *)*msg;
		pack_int32(p, NMSG_SWAP_INST_ADD);
	}

	free_data_list((struct data_list_t **)&new_lib_inst_list);
	new_lib_inst_list = NULL;
	*err = ERR_NO;

	generate_maps_inst_msg(us_inst);
	send_maps_inst_msg_to_all_targets();

msg_swap_inst_remove_exit:
	/* unlock list access */
	unlock_lib_list();

	return res;
}

void msg_swap_free_all_data(struct user_space_inst_t *us_inst)
{
	LOGI("new_lib_inst_list %p\n", new_lib_inst_list);
	if (new_lib_inst_list != NULL) {
		LOGI("free new_lib_inst_list start\n");
		free_data_list(&new_lib_inst_list);
		new_lib_inst_list = NULL;
		LOGI("free new_lib_inst_list finish\n");
	}

	LOGI("us_inst->lib_inst_list %p\n", us_inst->lib_inst_list);
	if (us_inst->lib_inst_list != NULL) {
		LOGI("free us_inst->lib_inst_list start\n");
		free_data_list(&us_inst->lib_inst_list);
		us_inst->lib_inst_list = NULL;
		LOGI("free us_isnt->lib_inst_list finish\n");
	}

	LOGI("us_inst->app_inst_list %p\n", us_inst->app_inst_list);
	if (us_inst->app_inst_list != NULL) {
		LOGI("free us_inst->app_inst_list start\n");
		free_data_list(&us_inst->app_inst_list);
		LOGI("free us_inst->app_isnt_list finish\n");
	}
}
