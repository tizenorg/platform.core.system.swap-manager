/*
 *  DA manager
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Anastasia Lyupa <a.lyupa@samsung.com>
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ui_viewer.h"
#include "da_protocol.h"
#include "swap_debug.h"
#include "smack.h"

int ui_viewer_set_smack_rules(const struct app_info_t *app_info)
{
	const char OBJECT[] = "swap";
	const char ACCESS_TYPE[] = "r";
	int ret = 0;

	ret = apply_smack_rules(app_info->app_id, OBJECT, ACCESS_TYPE);

	return ret;
}

int ui_viewer_set_app_info(const struct app_info_t *app_info)
{
	const char APP_INFO_FILE[] =
		"/sys/kernel/debug/swap/uihv/app_info";
	FILE *fp;
	int ret = 0, c = 0;
	uint64_t main_offset;

	main_offset = *(uint64_t*)app_info->setup_data.data;

	if (app_info->exe_path == NULL || !strlen(app_info->exe_path)) {
		LOGE("Executable path is not correct\n");
		ret = -EINVAL;
		goto fail;
	}

	fp = fopen(APP_INFO_FILE, "w");
	if (fp != NULL) {
		c = fprintf(fp, "0x%lx:%s\n", (unsigned long)main_offset,
			    app_info->exe_path);
		if (c < 0) {
			LOGE("Can't write to file: %s\n", APP_INFO_FILE);
			ret = -EIO;
		}
		fclose(fp);
	} else {
		LOGE("Can't open file: %s\n", APP_INFO_FILE);
		ret = -ENOENT;
	}
fail:
	return ret;
}
