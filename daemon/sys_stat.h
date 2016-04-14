/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim <jaewon81.lim@samsung.com>
 * Woojin Jung <woojin2.jung@samsung.com>
 * Juyoung Kim <j0.kim@samsung.com>
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
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
 * - S-Core Co., Ltd
 * - Samsung RnD Institute Russia
 *
 */


#ifndef _SYS_STAT_
#define _SYS_STAT_

#ifdef __cplusplus
extern "C" {
#endif

#define VOLTAGEFD					"/sys/class/power_supply/battery/voltage_now"

#define BRIGHTNESS_FILENAME			"brightness"
#define MAX_BRIGHTNESS_FILENAME		"max_brightness"
#define BRIGHTNESS_PARENT_DIR		"/sys/class/backlight"

#define EMUL_BRIGHTNESSFD			"/sys/class/backlight/emulator/brightness"
#define EMUL_MAX_BRIGHTNESSFD		"/sys/class/backlight/emulator/max_brightness"

#define MFCFD						"/sys/devices/platform/samsung-pd.0/power/runtime_status"

#define CPUDIR						"/sys/devices/system/cpu"
#define CPUFREQ_FILE				"cpufreq/stats/time_in_state"
#define CPUNUM_OF_FREQ				CPUDIR"/cpu0/"CPUFREQ_FILE

#ifdef DEVICE_ONLY
#define GEM_FILE				"/proc/dri/0/gem_names"
#else
#define GEM_FILE				"/sys/kernel/debug/dri/0/gem_names"
#endif


#define UMSFD						"/mnt/ums"
#define MMCBLKFD					"/dev/mmcblk1"
#define MMCFD						"/mnt/mmc"

#define PROCSTAT					"/proc/stat"
#define PROCMEMINFO					"/proc/meminfo"

#define FSINFO_TYPE_TOTAL			1
#define FSINFO_TYPE_FREE			2

#define NWTYPE_SIZE 128
#define MAXNAMESIZE 16


#include <stdint.h>
#include "da_protocol.h"

struct target_info_t {
	uint64_t sys_mem_size;
	uint64_t storage_size;
	uint32_t bluetooth_supp;
	uint32_t gps_supp;
	uint32_t wifi_supp;
	uint32_t camera_count;
	char network_type[NWTYPE_SIZE];
	uint32_t max_brightness;
	uint32_t cpu_core_count;
};

int get_system_info(struct system_info_t *sys_info);

int initialize_system_info(void);

int finalize_system_info(void);

int fill_target_info(struct target_info_t *target_info);

int init_sys_stat(void);
void uninit_sys_stat(void);
int sys_stat_prepare(void);
#ifdef __cplusplus
}
#endif

#endif
