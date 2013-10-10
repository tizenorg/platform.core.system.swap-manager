/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Nikita Kalyazin    <n.kalyazin@samsung.com>
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "da_protocol.h"
#include "da_data.h"
#include "da_inst.h"
#include "utils.h"
#include "elf.h"
#include "debug.h"

#define AWK_START "cat /proc/"
#define AWK_END_TIME "/stat | awk '{print $22}'"
#define AWK_END "/maps | grep 'r-x' | awk '{print $1, $6}' | grep ' /'"
#define AWK_END_PROCESS "/maps | grep 'r-x' | awk '{print $1, $6}' | grep "
#define AWK_SYSTEM_UP_TIME "cat /proc/uptime  | awk '{print $1}'"

// TODO don't make me cry
void write_process_info(int pid, uint64_t starttime)
{
	// TODO refactor this code
	char buf[1024];
	struct msg_data_t *msg = NULL;
	char *p = msg->payload;
	char *dep_count_p = NULL;
	int dep_count = 0;
	struct app_list_t *app = NULL;
	struct app_info_t *app_info = NULL;
	// TODO: add check for unknown type
	uint32_t binary_type = BINARY_TYPE_UNKNOWN;
	char binary_path[PATH_MAX];
	uint64_t start, end;
	float process_up_time;
	float sys_up_time;
	uint32_t up_sec = 0;
	uint32_t up_nsec = 0;
	char path[256];
	int fields;
	FILE *f;

	app_info = app_info_get_first(&app);
	if (app_info == NULL) {
		LOGE("No app info found\n");
		return;
	}
	binary_type = get_binary_type(app_info->exe_path);

	// TODO need check this result and return error
	dereference_tizen_exe_path(app_info->exe_path, path);
	get_build_dir(binary_path, app_info->exe_path);

	msg = malloc(64 * 1024);
	p = msg->payload;
	fill_data_msg_head(msg, NMSG_PROCESS_INFO, 0, 0);

	sprintf(buf, "%s%d%s%s", AWK_START, pid, AWK_END_PROCESS,path);

	f = popen(buf, "r");
	if (!f) {
		LOGW("open maps: %s\n", strerror(errno));
		free(msg);
		return;
	}

	fields = fscanf(f, "%llx-%llx %s", &start, &end, path);
	if (fields != 3) {
		start = 0;
		end = 0;
		LOGW("cannot find start-end values. fields=%d\n", fields);
	}
	pclose(f);

	if (starttime != 0) {
		up_sec = starttime / (1000 * 1000);
		up_nsec = starttime % (1000 * 1000) * 1000;
	} else {
		//get system uptime
		sprintf(buf, "%s", AWK_SYSTEM_UP_TIME);
		f = popen(buf, "r");
		if (!f) {
			LOGW("open uptime: %s\n", strerror(errno));
			free(msg);
			return;
		}

		fields = fscanf(f, "%f", &sys_up_time);
		if (fields != 1) {
			sys_up_time = 0;
		}
		pclose(f);

		//get process uptime
		sprintf(buf, "%s%d%s", AWK_START, pid, AWK_END_TIME);
		f = popen(buf, "r");
		if (!f) {
			LOGW("open stat: %s\n", strerror(errno));
			free(msg);
			return;
		}

		fields = fscanf(f, "%f", &process_up_time);
		if (fields != 1) {
			process_up_time = 0;
		} else {
			process_up_time = process_up_time / sysconf(_SC_CLK_TCK) ;
			process_up_time = sys_up_time - process_up_time;
		}
		pclose(f);

		up_sec = (int) process_up_time;
		up_nsec =  (int)( (process_up_time - (float) up_sec) * 1000000000 );

		up_sec = msg->sec - up_sec;
		if (msg->nsec < up_nsec){
			up_nsec = (1000000000 + msg->nsec) - up_nsec;
			up_sec--;
		} else {
			up_nsec = msg->nsec - up_nsec;
		}
	}

	LOGI(" process_up_time =  %f\nsec = %d; usec = %d\n"
			, process_up_time, up_sec, up_nsec);

	//Pack message
	pack_int(p, pid);
	pack_int(p, up_sec);
	pack_int(p, up_nsec);

	pack_int(p, start);
	pack_int(p, end);

	pack_int(p, binary_type);

	pack_str(p, binary_path);

	dep_count_p = p;
	pack_int(p, pid);

	sprintf(buf, "%s%d%s", AWK_START, pid, AWK_END);

	f = popen(buf, "r");
	if (!f) {
		LOGW("open maps: %s\n", strerror(errno));
		free(msg);
		return;
	}

	for (;;) {
		fields = fscanf(f, "%llx-%llx %s", &start, &end, path);
		if (fields == EOF) {
			break;
		}
		if (fields != 3) {
			LOGW("error string\n");
			break;
		}

		pack_int(p, start);
		pack_int(p, end);
		pack_str(p, path);
		dep_count++;
	}

	pack_int(dep_count_p, dep_count);
	msg->len = p - (char *)&msg->payload;
	write_to_buf(msg);

	pclose(f);

	free(msg);
}
