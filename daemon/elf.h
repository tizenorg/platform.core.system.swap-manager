/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
 * Dmitry Bogatov     <d.bogatov@samsung.com>
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
 * - Samsung RnD Institute Russia
 *
 */


#ifndef _ELF_
#define _ELF_
#include <limits.h>
enum binary_type_t {
	BINARY_TYPE_NO_PIE = 0,
	BINARY_TYPE_PIE = 1,
	BINARY_TYPE_UNKNOWN = -2,
	BINARY_TYPE_FILE_NOT_EXIST = -1
};

uint32_t get_binary_type(const char *path);
void get_build_dir(char builddir[PATH_MAX], const char *path);
#endif				/* _ELF_ */
