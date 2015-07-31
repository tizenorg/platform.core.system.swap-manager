/*
 *  DA manager
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *      Vyacheslav Cherkashin <v.cherkashin@samsung.com>
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

#ifndef FEATURE_MANAGER_C_H
#define FEATURE_MANAGER_C_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stddef.h>


int fm_init(void);
void fm_uninit(void);

int fm_start(void);
int fm_stop(void);

int fm_set(uint64_t f0, uint64_t f1);

int fm_app_add(uint32_t app_type, const char *id, const char *path,
               const void *data, size_t size);
void fm_app_clear(void);


#ifdef __cplusplus
}
#endif


#endif // FEATURE_MANAGER_C_H
