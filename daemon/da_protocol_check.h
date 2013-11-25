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

#include <stdint.h>

#include "daemon.h"

// Max values defines
#define APP_INFO_TYPE_MIN 0x0001
#define APP_INFO_TYPE_MAX 0x0003

#define CONF_SYSTRACE_PERIOD_MIN 10
#define CONF_SYSTRACE_PERIOD_MAX 1000

#define CONF_DATA_MSG_PERIOD_MIN 10
#define CONF_DATA_MSG_PERIOD_MAX 1000

#define US_APP_COUNT_MIN 0
#define US_APP_COUNT_MAX MAX_TARGET_COUNT

#define US_APP_INST_FUNC_MIN 0
#define US_APP_INST_FUNC_MAX 50000

#define US_FUNC_ARGS "bcdxpfw"
#define US_FUNC_RETURN "vnbcdxpfw"

#define US_APP_INST_LIB_MIN 0
#define US_APP_INST_LIB_MAX 100

int check_app_type(uint32_t app_type);
int check_app_id(uint32_t app_type, char *app_id);
int check_exec_path(char *path);
int check_conf_features(uint64_t feature0, uint64_t feature1);
int check_conf_systrace_period(uint32_t system_trace_period);
int check_conf_datamsg_period(uint32_t data_message_period);
int check_us_app_count(uint32_t app_count);
int check_us_app_inst_func_count(uint32_t func_count);
int check_us_inst_func_args(char *args);
int check_lib_inst_count(uint32_t lib_count);
int check_conf(struct conf_t *conf);
int check_us_inst_func_ret_type(char ret_type);
