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
#include "da_protocol_check.h"

#include <stdint.h>
#include <string.h>

//application checking functions
int check_app_type(uint32_t app_type)
{
	if ((app_type >= APP_INFO_TYPE_MIN) &&
		(app_type <= APP_INFO_TYPE_MAX)) {
		return 1;
	} else {
		LOGE("wrong value\n");
		return 0;
	}
}

static int is_pid_string_valid(const char *pid_str)
{
	char *tmp;
	long pid;

	if (pid_str[0] == '\0') /* no pid filtering */
		return 1;

	/* otherwise this should be a valid number */
	pid = strtol(pid_str, &tmp, 10);

	/* TODO: get max pid value from /proc/sys/kernel/pid_max */
	return *tmp == '\0' && pid > 0 && pid <= 32768;
}

int check_app_id(uint32_t app_type, char *app_id)
{
	int res = 0;
	char *p;
	switch (app_type){
		case APP_TYPE_TIZEN:
			res = 1;
			break;
		case APP_TYPE_RUNNING:
			if (!is_pid_string_valid(app_id))
				LOGE("wrong app id for APP_RUNNING\n");
			else
				res = 1;
			break;
		case APP_TYPE_COMMON:
			res = (strlen(app_id) == 0);
			if (!res)
				LOGE("wrong app id for APP_COMMON\n");
			break;
		case APP_TYPE_WEB:
			res = 1;
			break;
		default :
			LOGE("wrong app type\n");
			return 0;
			break;
	}
	return res;
}

int check_exec_path(char *path)
{
	int res = 1;
	struct stat buffer;

	if (!(res = (stat (path, &buffer) == 0)))
		LOGE("wrong exec path <%s>\n", path);

	return res;
}

//config checking functions
int check_conf_features(uint64_t feature0, uint64_t feature1)
{
	int res = 1;

	feature0 &= ~(uint64_t)FL_ALL_FEATURES;

	if (feature0 != 0) {
		LOGE("wrong features0 0x%016llX mask %016llX\n", feature0, (uint64_t)FL_ALL_FEATURES);
		res = 0;
	}

	feature1 &= ~(uint64_t)0;

	if (feature1 != 0) {
		LOGE("wrong features1 0x%016llX mask %016llX\n", feature1, (uint64_t)0);
		res = 0;
	}

	return res;
}


int check_conf_systrace_period(uint32_t system_trace_period)
{
	int res = 1;
	if ((system_trace_period < CONF_SYSTRACE_PERIOD_MIN) ||
		(system_trace_period > CONF_SYSTRACE_PERIOD_MAX))
	{
		LOGE("wrong system trace period value %u (0x%08X)\n",
		     (unsigned int) system_trace_period, system_trace_period);
		res = 0;
	}

	return res;
}

int check_conf_datamsg_period(uint32_t data_message_period)
{
	int res = 1;
	if ((data_message_period < CONF_DATA_MSG_PERIOD_MIN) ||
		(data_message_period > CONF_DATA_MSG_PERIOD_MAX))
	{
		LOGE("wrong data message period value %u (0x%08X)\n",
		     (unsigned int) data_message_period, data_message_period);
		res = 0;
	}

	return res;
}

//User space check functions
int check_us_app_count(uint32_t app_count)
{
	int res = 1;
	if (app_count > US_APP_COUNT_MAX) {
		LOGE("wrong user space app count %u (0x%08X)\n",
		     (unsigned int)app_count, app_count);
		res = 0;
	}

	return res;
}
//User space app inst check function
int check_us_app_inst_func_count(uint32_t func_count)
{
	int res = 1;
	if (func_count > US_APP_INST_FUNC_MAX) {
		LOGE("wrong US app inst func count %u (0x%08X)\n",
		     (unsigned int)func_count, func_count);
		res = 0;
	}

	return res;
}

static char *args_avail = US_FUNC_ARGS;
int check_us_inst_func_args(char *args)
{
	char *p;
	for (p = args; *p != 0; p++)
		if (strchr(args_avail, (int)*p) == NULL){
			LOGE("wrong args <%s> char <%c> <0x%02X>\n", args, (int)*p, (char)*p);
			return 0;
		}
	return 1;
}

static char *rets_avail = US_FUNC_RETURN;
int check_us_inst_func_ret_type(char ret_type)
{
	if (strchr(rets_avail, (int)ret_type) == NULL){
		LOGE("wrong ret type <%c> <0x%02X>\n", (int)ret_type, (char)ret_type);
		return 0;
	}
	return 1;
}

int check_lib_inst_count(uint32_t lib_count)
{
	int res = 1;
	if (lib_count > US_APP_INST_LIB_MAX) {
		LOGE("wrong US app inst lib count %u (0x%08X)\n",
		     (unsigned int)lib_count, lib_count);
		res = 0;
	}

	return res;
}

int check_conf(struct conf_t *conf)
{
	//Check features value
	if (!check_conf_features(conf->use_features0, conf->use_features1)) {
		LOGE("check features fail\n");
		return 0;
	}

	if (!check_conf_systrace_period(conf->system_trace_period)) {
		LOGE("system trace period error\n");
		return 0;
	}

	if (!check_conf_datamsg_period(conf->data_message_period)) {
		LOGE("data message period error\n");
		return 0;
	}

	return 1;
}
