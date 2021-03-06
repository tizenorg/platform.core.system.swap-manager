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

#ifndef _DEVICE_VCONF_H_
#define _DEVICE_VCONF_H_

int get_wifi_status(void);
int get_bt_status(void);
int get_gps_status(void);
int get_rssi_status(void);
int get_call_status(void);
int get_dnet_status(void);
int get_camera_status(void);
int get_sound_status(void);
int get_audio_status(void);
int get_vibration_status(void);

#endif /* _DEVICE_VCONF_H_ */
