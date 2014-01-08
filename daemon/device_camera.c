/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim		<jaewon81.lim@samsung.com>
 * Woojin Jung		<woojin2.jung@samsung.com>
 * Juyoung Kim		<j0.kim@samsung.com>
 * Nikita Kalyazin	<n.kalyazin@samsung.com>
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


#include <stdio.h>
#include <string.h>
#include "device_camera.h"

#define CAMCORDER_FILE "/usr/etc/mmfw_camcorder.ini"
#define CAMERA_COUNT_STR "DeviceCount"
#define BUFFER_MAX 1024

int get_camera_count(void)
{
	FILE* fp;
	int count = 0;
	int size;
	char buf[BUFFER_MAX];

	fp = fopen(CAMCORDER_FILE, "r");
	if (fp == NULL)
		return 0;

	size = strlen(CAMERA_COUNT_STR);
	while (fgets(buf, BUFFER_MAX, fp) != NULL) {
		if (strncmp(buf, CAMERA_COUNT_STR, size) == 0) {
			sscanf(buf, CAMERA_COUNT_STR " = %d", &count);
			break;
		}
	}

	fclose(fp);

	return count;
}
