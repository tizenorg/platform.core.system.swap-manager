/*
 *  DA manager
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Ruslan Soloviev <r.soloviev@samsung.com>
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
 * - Samsung Research Russia
 *
 */


#ifndef WSI_H_
#define WSI_H_

#include "da_inst.h"

enum web_prof_state_t {
	PROF_OFF,
	PROF_ON
};

int wsi_set_profile(const struct app_info_t *app_info);
int wsi_set_smack_rules(const struct app_info_t *app_info);
int wsi_enable_profiling(enum web_prof_state_t mode);

int wsi_start(void);
void wsi_stop(void);

#endif /* WSI_H_ */
