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

int check_app_id (uint32_t app_type, char *app_id)
{
	int res = 0;
	char *p;
	switch (app_type){
		case APP_TYPE_TIZEN:
			res = 1;
			break;
		case APP_TYPE_RUNNING:
			strtol(app_id, &p, 10);
			if ((*app_id != 0) && (*p == 0))
				res = 1;
			else
				LOGE("wrong app id for APP_RUNNING\n");
			break;
		case APP_TYPE_COMMON:
			res = (strlen(app_id) == 0);
			if (!res)
				LOGE("wrong app id for APP_COMMON\n");
			break;
		default :
			LOGE("wrong app type\n");
			return 0;
			break;
	}
	return res;
}

//config checking functions
int check_conf_features (uint64_t feature0, uint64_t feature1)
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
	if ((system_trace_period<CONF_SYSTRACE_PERIOD_MIN) ||
		(system_trace_period>CONF_SYSTRACE_PERIOD_MAX))
	{
		LOGE("wrong system trace period value %lu\n", system_trace_period);
		res = 0;
	}

	return res;
}

int check_conf_datamsg_period(uint32_t data_message_period)
{
	int res = 1;
	if ((data_message_period<CONF_DATA_MSG_PERIOD_MIN) ||
		(data_message_period>CONF_DATA_MSG_PERIOD_MAX))
	{
		LOGE("wrong data message period value %lu\n", data_message_period);
		res = 0;
	}

	return res;
}
