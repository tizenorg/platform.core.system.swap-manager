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

