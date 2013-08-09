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


// Max values defines
#define APP_INFO_TYPE_MIN 0x0001
#define APP_INFO_TYPE_MAX 0x0003

#define CONF_SYSTRACE_PERIOD_MIN 10
#define CONF_SYSTRACE_PERIOD_MAX 1000

#define CONF_DATA_MSG_PERIOD_MIN 10
#define CONF_DATA_MSG_PERIOD_MAX 1000
int check_app_type(uint32_t app_type);
int check_app_id (uint32_t app_type, char *app_id);
int check_conf_features (uint64_t feature0, uint64_t feature1);
