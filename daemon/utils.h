/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim <jaewon81.lim@samsung.com>
 * Woojin Jung <woojin2.jung@samsung.com>
 * Juyoung Kim <j0.kim@samsung.com>
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
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
 * - S-Core Co., Ltd
 * - Samsung RnD Institute Russia
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
#define DEBUG_LAUNCH_PRELOAD_PATH	"/usr/bin/debug_launchpad_preloading_preinitializing_daemon"
#define LAUNCH_APP_NAME			"launch_app"
#define WRT_LAUNCHER_PATH		"/usr/bin/wrt-launcher"
#define WRT_LAUNCHER_NAME		"wrt-launcher"
#define WRT_LAUNCHER_START		"-s"
#define WRT_LAUNCHER_START_DEBUG	"-d -s"
#define WRT_LAUNCHER_KILL		"-k"
#define LAUNCH_APP_SDK			"__AUL_SDK__"
#define DA_PRELOAD_EXEC			"DYNAMIC_ANALYSIS"
#define DA_PRELOAD_TIZEN		"LD_PRELOAD=/usr/lib/da_probe_tizen.so"
#define SHELL_CMD				"/bin/sh"


uint64_t	str_to_uint64(char* str);
int64_t		str_to_int64(char* str);

int remove_indir(const char *dirname);

int kill_app(const char *binary_path);

int is_same_app_process(char* appPath, int pid);

int exec_app_tizen(const char *app_id, const char *exec_path);
int exec_app_common(const char* exec_path);
int exec_app_web(const char *app_id);
void kill_app_web(const char *app_id);
float get_uptime(void);
pid_t find_pid_from_path(const char *path);
#ifdef __cplusplus
}
#endif

#endif
