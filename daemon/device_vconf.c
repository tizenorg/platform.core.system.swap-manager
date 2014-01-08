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


#include <vconf.h>
#include "debug.h"
#include "device_vconf.h"

int get_wifi_status(void)
{
	int wifi_status = 0;
	int res = 0;

	res = vconf_get_int(VCONFKEY_WIFI_STATE, &wifi_status);
	if (res < 0) {
		LOG_ONCE_W("get error #%d\n", res);
		wifi_status = VCONFKEY_WIFI_OFF;
	}

	return wifi_status;
}

int get_bt_status(void)
{
	int bt_status = 0;
	int res = 0;

	res = vconf_get_int(VCONFKEY_BT_STATUS, &bt_status);
	if (res < 0) {
		LOG_ONCE_W("get error #%d\n", res);
		bt_status = VCONFKEY_BT_STATUS_OFF;
	}

	return bt_status;
}

int get_gps_status(void)
{
	int gps_status = 0;
	int res = 0;

	res = vconf_get_int(VCONFKEY_LOCATION_ENABLED, &gps_status);
	if (res < 0) {
		LOG_ONCE_W("get error #%d\n", res);
		gps_status = VCONFKEY_LOCATION_GPS_OFF;
	} else if (gps_status != 0) {
		res = vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps_status);
		if (res < 0) {
			LOG_ONCE_W("get error #%d\n", res);
			gps_status = VCONFKEY_LOCATION_GPS_OFF;
		}
	}

	return gps_status;
}

int get_rssi_status(void)
{

	int flightmode_status;
	int res = 0;

	int rssi_status;
	res = vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE,
					&flightmode_status);
	if (res < 0) {
		LOG_ONCE_W("get err #%d <%s>\n", res,
			 VCONFKEY_TELEPHONY_FLIGHT_MODE);
		flightmode_status = 0;
	}

	if (!flightmode_status) {
		res = vconf_get_int(VCONFKEY_TELEPHONY_RSSI, &rssi_status);
		if (res < 0) {
			LOG_ONCE_W("rssi get err #%d\n", res);
			rssi_status = VCONFKEY_TELEPHONY_RSSI_0;
		}
	} else {
		rssi_status = VCONFKEY_TELEPHONY_RSSI_0;
	}

	return rssi_status;

	return 0;
}

int get_call_status(void)
{
	int call_status = 0;
	int res = 0;

	res = vconf_get_int(VCONFKEY_CALL_STATE, &call_status);
	if (res < 0) {
		LOG_ONCE_W("get err #%d\n", res);
		call_status = VCONFKEY_CALL_OFF;
	}

	return call_status;
}

int get_dnet_status(void)
{
	int dnet_status = 0;
	int res = 0;

	res = vconf_get_int(VCONFKEY_DNET_STATE, &dnet_status);
	if (res < 0) {
		LOG_ONCE_W("get err #%d <%s>\n", res, VCONFKEY_DNET_STATE);
		dnet_status = VCONFKEY_DNET_OFF;
	}

	return dnet_status;
}

int get_camera_status(void)
{
	int camera_status = 0;

	if (vconf_get_int(VCONFKEY_CAMERA_STATE, &camera_status) < 0) {
		camera_status = VCONFKEY_CAMERA_STATE_NULL;
	}

	return camera_status;
}

int get_sound_status(void)
{
	int sound_status = 0;
	int res = 0;

	res = vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL,
			     &sound_status);
	if (res < 0) {
		LOG_ONCE_W("get err #%d\n", res);
		sound_status = 0;
	}

	return sound_status;
}

int get_audio_status(void)
{
	int audio_state = 0;
	int res = 0;

	res = vconf_get_int(VCONFKEY_SOUND_STATUS,
			    &audio_state);
	if (res < 0) {
		LOG_ONCE_W("get err #%d\n", res);
		audio_state = 0;
	}

	return !!audio_state;
}

int get_vibration_status(void)
{
	int vibration_status = 0;
	int res = 0;

	res = vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL,
					&vibration_status);
	if (res < 0) {
		LOG_ONCE_W("get err #%d\n", res);
		vibration_status = 0;
	}

	return vibration_status;
}
