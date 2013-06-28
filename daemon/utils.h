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

#include <stdint.h>
#include <sys/types.h>

#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LAUNCH_APP_PATH			"/usr/bin/launch_app"
#define KILL_APP_PATH			"/usr/bin/pkill"
#define LAUNCH_APP_NAME			"launch_app"
#define LAUNCH_APP_SDK			"__AUL_SDK__"
#define DA_PRELOAD_EXEC			"DYNAMIC_ANALYSIS"
#define DA_PRELOAD(AppType)		AppType ? DA_PRELOAD_OSP : DA_PRELOAD_TIZEN
#define DA_PRELOAD_TIZEN		"LD_PRELOAD=/usr/lib/da_probe_tizen.so"
#define DA_PRELOAD_OSP			"LD_PRELOAD=/usr/lib/da_probe_osp.so"
#define BATT_LOG_FILE			"/home/developer/sdk_tools/da/battery/"
#define SHELL_CMD				"/bin/sh"

#define DA_INSTALL_PATH		"/home/developer/sdk_tools/da/da_install_path"
#define DA_BUILD_OPTION		"/home/developer/sdk_tools/da/da_build_option"
#define DA_BASE_ADDRESS		"/home/developer/sdk_tools/da/da_base_address"

uint64_t	str_to_uint64(char* str);
int64_t		str_to_int64(char* str);

int remove_indir(const char *dirname);

char* get_app_name(char* binary_path);

int exec_app(const char* exec_path, int app_type);

void kill_app(const char* binary_path);

pid_t find_pid_from_path(const char* path);

int get_app_type(char* appPath);
int get_executable(char* appPath, char* buf, int buflen);
int get_app_install_path(char *strAppInstall, int length);
int is_app_built_pie(void);
int get_app_base_address(int *baseAddress);
int is_same_app_process(char* appPath, int pid);

#ifdef __cplusplus
}
#endif

#endif
