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

#include "debug.h"
#include "da_protocol.h"
#include "da_protocol_inst.h"
#include "da_inst.h"
#include "da_protocol_check.h"

//----------------- hash
static uint32_t calc_lib_hash(struct us_lib_inst_t *lib)
{
	uint32_t hash = 0;
	char *p = lib->bin_path;
	while (*p != 0) {
		hash <<= 1;
		hash += *p;
		p++;
	}
	return hash;
}

static uint32_t calc_app_hash(struct app_info_t *app)
{
	uint32_t hash = 0;
	char *p = app->exe_path;
	while (*p != 0) {
		hash <<= 1;
		hash += *p;
		p++;
	}
	return hash;
}


//----------------------------------- parse -----------------------------------
static int parse_us_inst_func(struct msg_buf_t *msg, struct probe_list_t **dest)
{
	//probe format
	//name       | type   | len       | info
	//------------------------------------------
	//func_addr  | uint64 | 8         |
	//probe_type | char   | 1         |

	uint32_t size = 0;
	struct us_func_inst_plane_t *func = NULL;
	char type;
	uint64_t addr;

	size = sizeof(*func);

	if (!parse_int64(msg, &addr)) {
		LOGE("func addr parsing error\n");
		return 0;
	}

	if (!parse_int8(msg, &type)) {
		LOGE("func type parsing error\n");
		return 0;
	}

	switch (type) {
	case SWAP_RETPROBE:
		size += strlen(msg->cur_pos) + 1 + sizeof(char);
		break;
	case SWAP_FBI_PROBE:
		size += sizeof(uint32_t) + /* register number */
			sizeof(uint64_t) + /* register offset */
			sizeof(uint32_t) + /* data size */
			sizeof(uint64_t) + /* var id */
			sizeof(uint32_t);  /* pointer order */
		break;
	case SWAP_LD_PROBE:
		size += sizeof(uint64_t) + /* ld preload handler addr */
			strlen(msg->cur_pos + sizeof(uint64_t)) + /* probe args */
			1 + 					  /* '\0' */
			sizeof(char)				  /* ret type */;
		break;
	default:
		LOGE("wrong probe type <%u>\n", type);
		goto err_ret;
	}

	func = malloc(size);
	if (func == NULL) {
		LOGE("no memory\n");
		return 0;
	}

	func->probe_type = type;
	func->func_addr = addr;

	memcpy(&func->probe_info, msg->cur_pos, size - sizeof(*func));
	msg->cur_pos += size - sizeof(*func);

	*dest = new_probe();
	if (*dest == NULL) {
		LOGE("alloc new_probe error\n");
		goto err_ret;
	}
	(*dest)->size = size;
	(*dest)->func = func;
	return 1;

err_ret:
	free(func);
	return 0;
}

static int parse_func_inst_list(struct msg_buf_t *msg,
				struct data_list_t *dest)
{
	uint32_t i = 0, num = 0;
	struct probe_list_t *probe_el;

	if (!parse_int32(msg, &num) ||
		!check_us_app_inst_func_count(num))
	{
		LOGE("func num parsing error\n");
		return 0;
	}
	//parse user space function list

	parse_deb("app_int_num = %d\n", num);
	for (i = 0; i < num; i++) {
		parse_deb("app_int #%d\n", i);
		if (!parse_us_inst_func(msg, &probe_el)) {
			// TODO maybe need to free allocated memory up there
			LOGE("parse us inst func #%d failed\n", i + 1);
			return 0;
		}
		probe_list_append(dest, probe_el);
	}
	dest->func_num = num;
	return 1;
}

static int parse_inst_lib(struct msg_buf_t *msg, struct lib_list_t **dest)
{
	*dest = new_lib();
	if (*dest == NULL) {
		LOGE("lib alloc error\n");
		return 0;
	};

	if (!parse_string(msg, &((*dest)->lib->bin_path)) ||
	    !check_exec_path((*dest)->lib->bin_path))
	{
		LOGE("bin path parsing error\n");
		return 0;
	}

	if (!parse_func_inst_list(msg, (struct data_list_t *) *dest)) {
		LOGE("funcs parsing error\n");
		return 0;
	}

	(*dest)->size +=  strlen((*dest)->lib->bin_path) + 1 + sizeof((*dest)->func_num);
	(*dest)->hash = calc_lib_hash((*dest)->lib);
	return 1;

}

int parse_lib_inst_list(struct msg_buf_t *msg,
			uint32_t *num,
			struct lib_list_t **lib_list)
{
	uint32_t i = 0;
	struct lib_list_t *lib = NULL;
	if (!parse_int32(msg, num) ||
		!check_lib_inst_count(*num))
	{
		LOGE("lib num parsing error\n");
		return 0;
	}

	for (i = 0; i < *num; i++) {
		if (!parse_inst_lib(msg, &lib)) {
			// TODO maybe need free allocated memory up there
			LOGE("parse is inst lib #%d failed\n", i + 1);
			return 0;
		}
		data_list_append((struct data_list_t **)lib_list,
				 (struct data_list_t *)lib);
	}

	return 1;
}

int parse_inst_app(struct msg_buf_t *msg, struct app_list_t **dest)
{
	char *start, *end;
	struct app_info_t *app_info = NULL;
	*dest = new_app();

	if (*dest == NULL) {
		LOGE("lib alloc error\n");
		return 0;
	};

	app_info = (*dest)->app;
	start = msg->cur_pos;
	if (!parse_int32(msg, &app_info->app_type) ||
		!check_app_type(app_info->app_type))
	{
		LOGE("app type parsing error <0x%X>\n", app_info->app_type);
		return 0;
	}

	if (!parse_string(msg, &app_info->app_id) ||
		!check_app_id(app_info->app_type, app_info->app_id))
	{
		LOGE("app id parsing error\n");
		return 0;
	}
	if (!parse_string(msg, &app_info->exe_path) ||
	    ((app_info->app_type != APP_TYPE_WEB) &&
	     ((app_info->app_type != APP_TYPE_RUNNING) ||
	      (app_info->app_id[0] != '\0')) &&
	     !check_exec_path(app_info->exe_path)))
	{
		LOGE("exec path parsing error\n");
		return 0;
	}
	end = msg->cur_pos;

	if (!parse_func_inst_list(msg, (struct data_list_t *)*dest)) {
		LOGE("funcs parsing error\n");
		return 0;
	}

	(*dest)->size += (end - start) + sizeof((*dest)->func_num);
	(*dest)->hash = calc_app_hash(app_info);
	return 1;
}

int parse_app_inst_list(struct msg_buf_t *msg,
			uint32_t *num,
			struct app_list_t  **app_list)
{
	uint32_t i = 0;
	struct app_list_t *app = NULL;
	if (!parse_int32(msg, num) ||
		!check_lib_inst_count(*num))
	{
		LOGE("app num parsing error\n");
		return 0;
	}

	parse_deb("app_int_num = %d\n", *num);
	for (i = 0; i < *num; i++) {
		parse_deb("app_int #%d\n", i);
		if (!parse_inst_app(msg, &app)) {
			// TODO maybe need free allocated memory up there
			LOGE("parse is inst app #%d failed\n", i + 1);
			return 0;
		}
		data_list_append((struct data_list_t **)app_list,
				 (struct data_list_t *)app);
	}

	if (!parse_replay_event_seq(msg, &prof_session.replay_event_seq)) {
		LOGE("replay parsing error\n");
		return 0;
	}

	return 1;
}

