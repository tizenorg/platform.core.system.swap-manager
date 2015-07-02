/*
 *  DA manager
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Lyupa Anastasia <a.lyupa@samsung.com>
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

#ifndef _UI_VIEWER_UTILS_
#define _UI_VIEWER_UTILS_

#include <stdint.h>
#include <Eina.h>

#include "app_protocol.h" /* from swap-probe-devel package */

#include "ui_viewer_data.h"

#define DA_LOG_MAX		4096
#define TMP_DIR "/tmp/da"

#define __unused __attribute__((unused))

typedef struct
{
	int type;
	int length;
	char data[DA_LOG_MAX];
} log_t;

// ========================= print log =====================================
#define PRINTMSG(...)	print_log_fmt(APP_MSG_MSG, __FUNCTION__, __LINE__, __VA_ARGS__)
#define PRINTWRN(...)	print_log_fmt(APP_MSG_WARNING, __FUNCTION__, __LINE__, __VA_ARGS__)
#define PRINTERR(...)	print_log_fmt(APP_MSG_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)

#define INIT_INFO						\
		info.host_ip = 0;				\
		info.host_port = 0;				\
		info.msg_total_size = 0;			\
		info.msg_pack_size = 0;				\
		info.sock = NULL;				\
		info.msg_buf = (char *)""

typedef struct {
	uint32_t host_port;
	uint32_t host_ip;
	struct sockaddr *sock;

	uint64_t msg_total_size;
	uint32_t msg_pack_size;
	char *msg_buf;

} info_t;

void reset_pid_tid();
pid_t _getpid();
pid_t _gettid();
bool print_log_fmt(int msgType, const char *func_name, int line, ...);
bool print_log_str(int msgType, char *st);
bool print_log_ui_viewer_info_list(int msgType, Eina_List *ui_obj_info_list);
bool print_log_ui_obj_properties(int msgType, ui_obj_prop_t *prop);
bool print_log_ui_obj_rendering_time(int msgType, Eina_List *ui_obj_rendering_list);
bool printLog(log_t* log, int msgType);
int __redirect_std(void);
const char *msg_code_to_srt(enum AppMessageType type);

int captureScreen(char *screenshot_path);

#endif /* _UI_VIEWER_UTILS_ */
