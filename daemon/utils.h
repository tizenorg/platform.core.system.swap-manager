/*
*  DA manager
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: 
*
* Jaewon Lim <jaewon81.lim@samsung.com>
* Woojin Jung <woojin2.jung@samsung.com>
* Juyoung Kim <j0.kim@samsung.com>
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
* - S-Core Co., Ltd
*
*/ 

#ifndef _UTILS_
#define _UTILS_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>

#define LAUNCH_APP_PATH			"/usr/bin/launch_app"
#define KILL_APP_PATH			"/usr/bin/pkill"
#define LAUNCH_APP_NAME			"launch_app"
#define DA_PRELOAD_EXEC			"__AUL_SDK_DYNAMIC_ANALYSIS"
#define DA_PRELOAD(AppType)		AppType ? DA_PRELOAD_OSP : DA_PRELOAD_TIZEN
#define DA_PRELOAD_TIZEN		"LD_PRELOAD=/usr/lib/da_probe_tizen.so"
#define DA_PRELOAD_OSP			"LD_PRELOAD=/usr/lib/da_probe_osp.so"
#define BATT_LOG_FILE			"/home/developer/sdk_tools/da/battery/"
#define SHELL_CMD				"/bin/sh"

enum ApplicationType
{
	APP_TYPE_TIZEN = 0,
	APP_TYPE_OSP = 1
};

char* get_app_name(char* binary_path);

int exec_app(const char* exec_path, int app_type);

void kill_app(const char* binary_path);

pid_t find_pid_from_path(const char* path);

int create_open_batt_log(const char* app_name);

int get_batt_fd();

int write_batt_log(const char* message);

void close_batt_fd();

#if DEBUG
void write_log();
#endif


#ifdef __cplusplus
}
#endif

#endif
