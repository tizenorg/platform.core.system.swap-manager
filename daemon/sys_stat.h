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

#define CHARGERFD					"/sys/class/power_supply/battery/charge_now"
#define VOLTAGEFD					"/sys/class/power_supply/battery/voltage_now"

#define BRIGHTNESS_FILENAME			"brightness"
#define MAX_BRIGHTNESS_FILENAME		"max_brightness"
#define BRIGHTNESS_PARENT_DIR		"/sys/class/backlight"

#define EMUL_BRIGHTNESSFD			"/sys/class/backlight/emulator/brightness"
#define EMUL_MAX_BRIGHTNESSFD		"/sys/class/backlight/emulator/max_brightness"


//#define AUDIOFD						"/sys/devices/platform/soc-audio/dapm_widget"
#define AUDIOFD						"/sys/devices/platform/soc-audio/WM1811 Voice/dapm_widget"

#define MFCFD						"/sys/devices/platform/s3c-mfc/mfc/mfc_status"

#define FREQFD						"/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"

#define CPUDIR						"/sys/devices/system/cpu"
#define CPUFREQ_FILE				"cpufreq/stats/time_in_state"
#define CPUNUM_OF_FREQ				CPUDIR"/cpu0/"CPUFREQ_FILE


#define MEMINFOFD					"/sys/class/memnotify/meminfo"
#define UMSFD						"/mnt/ums"
#define MMCBLKFD					"/dev/mmcblk1"
#define MMCFD						"/mnt/mmc"

#define PROCSTAT					"/proc/stat"
#define PROCMEMINFO					"/proc/meminfo"
#define PROCCPUINFO					"/proc/cpuinfo"

#define MEM_TYPE_TOTAL				1
#define MEM_TYPE_USED				2
#define MEM_TYPE_FREE				3

#define FSINFO_TYPE_TOTAL			1
#define FSINFO_TYPE_FREE			2

#define MAXNAMESIZE 16

#include <stdint.h>
#include "da_protocol.h"

typedef unsigned long long tic_t;

typedef struct {
	unsigned int pid;
	char command[MAXNAMESIZE];
	char state;
	int ppid;
	int pgrp;
	int sid;
	int tty_nr;
	int tty_pgrp;
	unsigned long flags;
	unsigned long minor_fault;
	unsigned long cminor_fault;
	unsigned long major_fault;
	unsigned long cmajor_fault;
	unsigned long long utime;
	unsigned long long stime;
	unsigned long long cutime;
	unsigned long long cstime;
	long priority;
	long nice;
	int numofthread;
	long dummy;
	unsigned long long start_time;
	unsigned long vir_mem;
	unsigned long sh_mem;
	long res_memblock;
	unsigned long pss;
} proc_t;

typedef struct {
	unsigned long freq;
	tic_t tick;
	tic_t tick_sav;
} cpufreq_t;

typedef struct {
	tic_t u, n, s, i, w, x, y, z;
	tic_t u_sav, n_sav, s_sav, i_sav, w_sav, x_sav, y_sav, z_sav;
	unsigned int id;		// cpu id
	float cpu_usage;		// cpu load for this core
	int sav_load_index;		// saved cpu load sampling index
	int cur_load_index;		// current cpu load sampling index
	cpufreq_t* pfreq;		// frequency information of cpu
	int sav_freq_index;		// sav cpu frequency sampling index
	int cur_freq_index;		// current cpu frequency sampling index
	long long idle_ticks;
	long long total_ticks;
} CPU_t;	// for each cpu core

typedef struct _mem_t {
	const char* name;		// memory slot name
	unsigned long* slot;	// memory value slot
} mem_t;

struct target_info_t {
	uint64_t sys_mem_size;
	uint64_t storage_size;
	uint32_t bluetooth_supp;
	uint32_t gps_supp;
	uint32_t wifi_supp;
	uint32_t camera_count;
	char *network_type;
	uint32_t max_brightness;
	uint32_t cpu_core_count;
};

int get_system_info(struct system_info_t *sys_info, int *pidarray, int pidcount);

int get_device_info(char* buffer, int buffer_len);

int get_file_status(int* pfd, const char* filename);

int initialize_system_info();

int finalize_system_info();

int fill_target_info(struct target_info_t *target_info);

int init_system_file_descriptors();
#ifdef __cplusplus
}
#endif

#endif
