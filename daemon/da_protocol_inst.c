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

#include "swap_debug.h"
#include "da_protocol.h"
#include "da_protocol_inst.h"
#include "da_inst.h"
#include "da_protocol_check.h"
#include "ld_preload_probe_lib.h"
#include "cpp/features/feature_manager_c.h"

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

struct fbi_step_t {
	uint8_t pointer_order;
	uint64_t data_offset;
} __attribute__ ((packed));

struct fbi_probe_t {
	uint64_t varid;
	uint64_t reg_offset;
	uint8_t reg_num;
	uint32_t data_size;
	uint8_t steps_count;
	struct fbi_step_t fbi_step[0];
} __attribute__ ((packed));

static uint32_t get_fbi_probe_size(struct msg_buf_t *msg)
{
	char *p = NULL;
	uint8_t vars_c, i;
	uint32_t size = 0;
	struct fbi_probe_t *fbi_probe;

	if (msg == NULL || msg->cur_pos == NULL) {
		LOGE("wrong msg structure\n");
		goto err_ret;
	}
	p = msg->cur_pos;

	/* extract valuses count */
	vars_c = *(uint8_t *)p;
	p += sizeof(vars_c);

	parse_deb("fbi probe count = %d \n", vars_c);

	for (i = 0; i != vars_c; i++) {
		fbi_probe = (struct fbi_probe_t *)p;
		size = /* header */
		       sizeof(*fbi_probe) +
		       /* steps size */
		       fbi_probe->steps_count * sizeof(fbi_probe->fbi_step[0]);

		if (p + size > msg->end) {
			LOGE("\nfbi probe #%d parsing error:\n"
			     "\tvarid       %016llX\n"
			     "\treg_offset  %016llX\n"
			     "\treg_num     %02x\n"
			     "\tdata_size   %016lX\n"
			     "\tsteps_count %02x\n",
			     i + 1,
			     fbi_probe->varid,
			     fbi_probe->reg_offset,
			     (int)fbi_probe->reg_num,
			     fbi_probe->data_size,
			     (int)fbi_probe->steps_count
			     );
			goto err_ret;
		}
		p += size;
	}

	return (uint32_t)(p - msg->cur_pos);
err_ret:
	return 0;
}

//----------------------------------- parse -----------------------------------
static int parse_us_inst_func(struct msg_buf_t *msg, struct probe_list_t **dest)
{
	//probe format
	//name       | type   | len       | info
	//------------------------------------------
	//func_addr  | uint64 | 8         |
	//probe_type | char   | 1         |

	uint32_t size = 0, tmp_size = 0;
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
		tmp_size = get_fbi_probe_size(msg);
		if (tmp_size == 0) {
			LOGE("probe parsing error\n");
			goto err_ret;
		}
		size += tmp_size;
		break;
	case SWAP_LD_PROBE:
		size += sizeof(uint64_t) + /* ld preload handler addr */
			sizeof(char);		  /* ld probe type */
		break;
	case SWAP_WEBPROBE:
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
		goto free_func_ret;
	}
	(*dest)->size = size;
	(*dest)->func = func;
	return 1;

free_func_ret:
	free(func);
err_ret:
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
	int res = 1;
	*dest = new_lib();
	if (*dest == NULL) {
		LOGE("lib alloc error\n");
		res = 0;
		goto exit;
	};

	if (!parse_string(msg, &((*dest)->lib->bin_path)) ||
	    !check_exec_path((*dest)->lib->bin_path))
	{
		LOGE("bin path parsing error\n");
		goto exit_free_err;
	}

	if (!parse_func_inst_list(msg, (struct data_list_t *) *dest)) {
		LOGE("funcs parsing error\n");
		goto exit_free_err;
	}

	(*dest)->size +=  strlen((*dest)->lib->bin_path) + 1 + sizeof((*dest)->func_num);
	(*dest)->hash = calc_lib_hash((*dest)->lib);

	goto exit;

exit_free_err:
	res = 0;
	free(*dest);

exit:
	return res;

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

static int parse_inst_app_setup_data(struct msg_buf_t *msg,
                                    struct app_info_t *app_info)
{
	switch (app_info->app_type) {
	case APP_TYPE_TIZEN: {
		void *data = msg->cur_pos;
		enum { data_len = 8 };
		uint64_t val;

		/* skip 8 bytes */
		if (!parse_int64(msg, &val)) {
			LOGE("main address parsing error\n");
			return -EINVAL;
		}

		app_info->setup_data.size = data_len;
		memcpy(app_info->setup_data.data, data, data_len);
		return 0;
	}
	case APP_TYPE_RUNNING:
	case APP_TYPE_COMMON:
	case APP_TYPE_WEB:
		app_info->setup_data.size = 0;
		return 0;
	default:
               return -EINVAL;
	}

	return 0;
}

int parse_inst_app(struct msg_buf_t *msg, struct app_list_t **dest)
{
	int res = 1;
	char *start, *end;
	struct app_info_t *app_info = NULL;
	*dest = new_app();

	if (*dest == NULL) {
		LOGE("lib alloc error\n");
		res = 0;
		goto exit;
	};

	app_info = (*dest)->app;
	start = msg->cur_pos;
	if (!parse_int32(msg, &app_info->app_type) ||
		!check_app_type(app_info->app_type))
	{
		LOGE("app type parsing error <0x%X>\n", app_info->app_type);
		goto exit_free_err;
	}

	if (!parse_string(msg, &app_info->app_id) ||
		!check_app_id(app_info->app_type, app_info->app_id))
	{
		LOGE("app id parsing error\n");
		goto exit_free_err;
	}

	if (!parse_string(msg, &app_info->exe_path) ||
	    ((app_info->app_type != APP_TYPE_WEB) &&
	     ((app_info->app_type != APP_TYPE_RUNNING) ||
	      (app_info->app_id[0] != '\0')) &&
	     !check_exec_path(app_info->exe_path)))
	{
		LOGE("exec path parsing error\n");
		goto exit_free_err;
	}
	end = msg->cur_pos;

	if (!parse_func_inst_list(msg, (struct data_list_t *)*dest)) {
		LOGE("funcs parsing error\n");
		goto exit_free_err;
	}

	if (parse_inst_app_setup_data(msg, app_info)) {
		LOGE("setup data parsing error\n");
		goto exit_free_err;
	}

	(*dest)->size += (end - start) + sizeof((*dest)->func_num);
	(*dest)->hash = calc_app_hash(app_info);
	goto exit;

exit_free_err:
	res = 0;
	free(*dest);
exit:
	return res;
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
		int err;
		struct app_info_t *info;

		parse_deb("app_int #%d\n", i);
		if (!parse_inst_app(msg, &app)) {
			// TODO maybe need free allocated memory up there
			LOGE("parse is inst app #%d failed\n", i + 1);
			return 0;
		}

		info = app->app;
		err = fm_app_add(info->app_type, info->app_id, info->exe_path,
				 info->setup_data.data, info->setup_data.size);
		if (err) {
			LOGE("add app, ret=%d\n", err);
			free_app(app);
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
/* ld probes */
#define NOFEATURE 0x123456
#include "ld_preload_types.h"
struct ld_preload_probe_t {
	uint64_t orig_addr;
	uint8_t probe_type;
	uint64_t handler_addr;
	uint8_t preload_type;
} __attribute__ ((packed));

static int feature_add_func_inst_list(struct ld_lib_list_el_t ld_lib,
				      struct data_list_t *dest)
{
	uint32_t i = 0, num = 0;
	struct probe_list_t *probe_el;
	struct ld_preload_probe_t *func = NULL;

	num = ld_lib.probe_count;

	if (!check_us_app_inst_func_count(num)) {
		LOGE("ld probe count is wrong\n");
		return 0;
	}
	//parse user space function list

	LOGI("app_int_num = %d\n", num);
	for (i = 0; i < num; i++) {
		parse_deb("app_int #%d\n", i);
		probe_el = new_probe();

		if (probe_el == NULL) {
			LOGE("probe alloc error\n");
			return 0;
		}

		func = malloc(sizeof(struct ld_preload_probe_t));

		if (func == NULL) {
			LOGE("func alloc error\n");
			free(probe_el);
			return 0;
		}

		func->orig_addr = ld_lib.probes[i].orig_addr;
		func->probe_type = SWAP_LD_PROBE;
		func->handler_addr = ld_lib.probes[i].handler_addr;
		func->preload_type = ld_lib.probes[i].probe_type;

		probe_el->size = sizeof(struct ld_preload_probe_t);
		probe_el->func = (struct us_func_inst_plane_t *)func;

		LOGI("app_int #%d size = %lu\n", i, dest->size);
		probe_list_append(dest, probe_el);
	}
	dest->func_num = num;
	return 1;
}

static int feature_add_inst_lib(struct ld_lib_list_el_t ld_lib,
				struct lib_list_t **dest)
{
	*dest = new_lib();
	if (*dest == NULL) {
		LOGE("lib alloc error\n");
		return 0;
	};

	if (!check_exec_path(ld_lib.lib_name)) {
		LOGE("bin path parsing error\n");
		return 0;
	}

	(*dest)->lib->bin_path = strdup(ld_lib.lib_name);

	if (!feature_add_func_inst_list(ld_lib, (struct data_list_t *)*dest)) {
		LOGE("funcs parsing error\n");
		return 0;
	}

	(*dest)->size += strlen((*dest)->lib->bin_path) + 1 + sizeof((*dest)->func_num);
	(*dest)->hash = calc_lib_hash((*dest)->lib);
	return 1;

}

int feature_add_lib_inst_list(struct ld_feature_list_el_t *ld_lib_list,
			      struct lib_list_t **lib_list)
{

	uint32_t i = 0, num;
	struct lib_list_t *lib = NULL;

	num = ld_lib_list->lib_count;
	if (!check_lib_inst_count(num)) {
		LOGE("lib num parsing error\n");
		return 0;
	}

	for (i = 0; i < num; i++) {
		LOGI(">add lib #%d <%s> probes_count=%lu\n", i, ld_lib_list->libs[i].lib_name, ld_lib_list->libs[i].probe_count);
		if (!feature_add_inst_lib(ld_lib_list->libs[i], &lib)) {
			// TODO maybe need free allocated memory up there
			LOGE("add LD lib #%d failed\n", i + 1);
			return 0;
		}
		data_list_append((struct data_list_t **)lib_list,
				 (struct data_list_t *)lib);
	}

	return 1;
}

static int create_preload_probe_func(struct probe_list_t **probe,
				     unsigned long probe_addr,
				     char probe_type)
{
	struct us_func_inst_plane_t *func = NULL;

	func = malloc(sizeof(*func));
	if (func == NULL) {
		LOGE("no memory for probe func\n");
		return -ENOMEM;
	}

	func->func_addr = probe_addr;
	func->probe_type = probe_type;

	(*probe)->size = sizeof(*func);
	(*probe)->func = func;

	return 0;
}

int add_preload_probes(struct lib_list_t **lib_list)
{
	struct lib_list_t *preload_lib;
	struct probe_list_t
	    *get_caller_probe,
	    *get_call_type_probe,
	    *write_msg_probe;
	int ret = 0;
	struct us_func_inst_plane_t *func = NULL;

	preload_lib = new_lib();
	if (preload_lib == NULL) {
		LOGE("preload lib alloc error\n");
		ret = 0;
		goto exit_fail;
	}

	get_caller_probe = new_probe();
	get_call_type_probe = new_probe();
	write_msg_probe = new_probe();
	if (get_caller_probe == NULL || get_call_type_probe == NULL || write_msg_probe == NULL) {
		LOGE("probe alloc error\n");
		ret = 0;
		goto free_caller_probe;
	}

	preload_lib->lib->bin_path = probe_lib;
	preload_lib->func_num = 3;

	/* Add get_caller probe */
	ret = create_preload_probe_func(&get_caller_probe, get_caller_addr, 4);
	if (ret != 0)
		goto free_caller_probe;

	probe_list_append(preload_lib, get_caller_probe);

	/* Add get_call_type probe */
	ret = create_preload_probe_func(&get_call_type_probe, get_call_type_addr, 5);
	if (ret != 0)
		goto free_call_type_probe;

	probe_list_append(preload_lib, get_call_type_probe);

	/* Add write_msg probe */
	ret = create_preload_probe_func(&write_msg_probe, write_msg_addr, 6);
	if (ret != 0)
		goto free_write_msg_probe;

	probe_list_append(preload_lib, write_msg_probe);

	preload_lib->func_num = 3;
	preload_lib->size += strlen(preload_lib->lib->bin_path) + 1 + sizeof(preload_lib->func_num);
	preload_lib->hash = calc_lib_hash(preload_lib->lib);

	data_list_append((struct data_list_t **)lib_list, (struct data_list_t *)preload_lib);

	return 1;

free_caller_probe:
	free(get_caller_probe);
free_call_type_probe:
	free(get_call_type_probe);
free_write_msg_probe:
	free(write_msg_probe);

	preload_lib->lib->bin_path = NULL;
	free_lib(preload_lib);
exit_fail:
	return ret;
}
