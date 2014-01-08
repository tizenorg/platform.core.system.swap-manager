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


#include <system_info.h>
#include <runtime_info.h>
#include <telephony_network.h>
#include <call.h>
#include "device_system_info.h"

static int is_available(const char *path)
{
	bool res;

	system_info_get_platform_bool(path, &res);

	return res;
}

int is_cdma_available(void)
{
	return is_available("tizen.org/feature/network.telephony.service.cdma");
}

int is_edge_available(void)
{
	return is_available("tizen.org/feature/network.telephony.service.edge");
}

int is_gprs_available(void)
{
	return is_available("tizen.org/feature/network.telephony.service.gprs");
}

int is_gsm_available(void)
{
	return is_available("tizen.org/feature/network.telephony.service.gsm");
}

int is_hsdpa_available(void)
{
	return is_available("tizen.org/feature/network.telephony.service.hsdpa");
}

int is_hspa_available(void)
{
	return is_available("tizen.org/feature/network.telephony.service.hspa");
}

int is_hsupa_available(void)
{
	return is_available("tizen.org/feature/network.telephony.service.hsupa");
}

int is_umts_available(void)
{
	return is_available("tizen.org/feature/network.telephony.service.umts");
}

int is_lte_available(void)
{
	return is_available("tizen.org/feature/network.telephony.service.lte");
}

int is_bluetooth_available(void)
{
	return is_available("tizen.org/feature/network.bluetooth");
}

int is_gps_available(void)
{
	return is_available("tizen.org/feature/location.gps");
}

int is_wifi_available(void)
{
	return is_available("tizen.org/feature/network.wifi");
}
