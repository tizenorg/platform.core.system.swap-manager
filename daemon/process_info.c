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
#include "elf.h"
#include "debug.h"

#define AWK_START "cat /proc/"
#define AWK_END "/maps | grep 'r-x' | awk '{print $1, $6}' | grep ' /'"
#define AWK_END_PROCESS "/maps | grep 'r-x' | awk '{print $1, $6}' | grep "

void write_process_info(int pid, uint64_t starttime)
{
	char buf[1024];
	struct msg_data_t *msg = malloc(64 * 1024);
	char *p = &msg->payload;
	char *dep_count_p = NULL;
	int dep_count = 0;
	/* uint32_t sec = starttime / 10000; */
	/* uint32_t nsec = starttime % 10000 * 100 * 1000; */
	// TODO: add check for unknown type
	uint32_t binary_type = get_binary_type(prof_session.app_info.exe_path);
	char binary_path[PATH_MAX];
	uint64_t start, end;
	char path[256];
	int fields;
	FILE *f;

	get_build_dir(binary_path, prof_session.app_info.exe_path);

	fill_data_msg_head(msg, NMSG_PROCESS_INFO, 0, 0);

	sprintf(buf, "%s%d%s%s", AWK_START, pid, AWK_END_PROCESS,
		prof_session.app_info.exe_path);

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
	}
	pclose(f);

	pack_int(p, pid);
	pack_int(p, msg->sec);
	pack_int(p, msg->nsec);

	pack_int(p, start);
	pack_int(p, end);

	pack_int(p, prof_session.app_info.app_type);

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
