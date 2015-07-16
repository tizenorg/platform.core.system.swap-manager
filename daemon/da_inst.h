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

#ifndef __DA_INST_H__
#define __DA_INST_H__

#include <stdint.h>
#include <sys/types.h>
#include "da_protocol.h"

//LISTS
struct probe_list_t {
	void *next;
	void *prev;
	uint32_t size;				//for message generate
	struct us_func_inst_plane_t *func;
};

struct lib_list_t {
	void *next;
	void *prev;
	uint32_t hash;
	uint32_t size;				//for message generate
	struct us_lib_inst_t *lib;
	uint32_t func_num;
	struct probe_list_t *list;
};

struct app_list_t {
	void *next;
	void *prev;
	uint32_t hash;
	uint32_t size;				//for message generate
	struct app_info_t *app;
	uint32_t func_num;
	struct probe_list_t *list;
};


struct data_list_t {
	void *next;
	void *prev;
	uint32_t hash;
	uint32_t size;				//for message generate
	void *data;
	uint32_t func_num;
	struct probe_list_t *list;
};

typedef int (cmp_data_f) (struct data_list_t *el_1, struct data_list_t *el_2);
typedef char *(*pack_head_t) (char *to, void *data);

extern int msg_swap_inst_remove(struct msg_buf_t *data, struct user_space_inst_t *us_inst,
				struct msg_t **msg, enum ErrorCode *err);
extern int msg_swap_inst_add(struct msg_buf_t *data, struct user_space_inst_t *us_inst,
			     struct msg_t **msg, enum ErrorCode *err);
extern int msg_start(struct msg_buf_t *data, struct user_space_inst_t *us_inst,
		     struct msg_t **msg, enum ErrorCode *err);

struct probe_list_t *new_probe(void);
struct lib_list_t *new_lib(void);
struct app_list_t *new_app(void);
int probe_list_append(struct data_list_t *to, struct probe_list_t *from);
int data_list_append(struct data_list_t **to, struct data_list_t *from);
void free_data_list(struct data_list_t **data);
void free_app(struct app_list_t *app);
void free_lib(struct lib_list_t *lib);

void app_list_rm_probes_by_addr(struct app_list_t *app_list, uint64_t addr);

struct app_info_t *app_info_get_first(struct app_list_t **app_list);
struct app_info_t *app_info_get_next(struct app_list_t **app_list);

struct target; // move
void send_maps_inst_msg_to(struct target *t);

#endif /* __DA_INST_H__*/
