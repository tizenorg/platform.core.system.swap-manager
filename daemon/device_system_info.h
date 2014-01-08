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

#ifndef _DEVICE_SYSTEM_INFO_H_
#define _DEVICE_SYSTEM_INFO_H_

int is_cdma_available(void);
int is_edge_available(void);
int is_gprs_available(void);
int is_gsm_available(void);
int is_hsdpa_available(void);
int is_hsupa_available(void);
int is_hspa_available(void);
int is_umts_available(void);
int is_lte_available(void);

int is_bluetooth_available(void);
int is_gps_available(void);
int is_wifi_available(void);

#endif /* _DEVICE_SYSTEM_INFO_H_ */
