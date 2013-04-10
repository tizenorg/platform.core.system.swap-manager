/*
*  DA manager
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: 
*
* Jaewon Lim <jaewon81.lim@samsung.com>
* Woojin Jung <woojin2.jung@samsung.com>
* Juyoung Kim <j0.kim@samsung.com>
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
*
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#ifndef LOCALTEST
#include <system_info.h>
#include <runtime_info.h>
#include <telephony_network.h>
#include <call.h>
#endif

#define USE_VCONF

#ifdef USE_VCONF
#include "vconf.h"
#endif

#include "sys_stat.h"
#include "daemon.h"

// defines for runtime environment
#define FOR_EACH_CPU

#define BUFFER_MAX			1024
#define LARGE_BUFFER		512
#define MIDDLE_BUFFER		256
#define SMALL_BUFFER		64
#define PROCPATH_MAX		32
#define STATUS_STRING_MAX	16
#define MAX_NUM_OF_FREQ		16

#define MEM_SLOT_TOTAL		0
#define MEM_SLOT_FREE		1
#define MEM_SLOT_BUFFER		2
#define MEM_SLOT_CACHED		3
#define MEM_SLOT_MAX		4

#define MIN_TICKS_FOR_LOAD	8
#define MIN_TOTAL_TICK		10
#define SYS_INFO_TICK		100	// TODO : change to (Hertz * profiling period)

#define CPUMHZ		"cpu MHz"
#define DA_PROBE_TIZEN_SONAME		"da_probe_tizen.so"
#define DA_PROBE_OSP_SONAME			"da_probe_osp.so"

enum PROCESS_DATA
{
	PROCDATA_STAT,
	PROCDATA_SMAPS
};

// declared by greatim
static int Hertz = 0;
static int num_of_cpu = 0;
static int num_of_freq = 0;
static unsigned long mem_slot_array[MEM_SLOT_MAX];
static CPU_t* cpus = NULL;
static unsigned long probe_so_size = 0;

// daemon api : get status from file
// pfd must not be null
int get_file_status(int* pfd, const char* filename)
{
	int status = 0;

#ifndef LOCALTEST
	if(likely(pfd != NULL))
	{
		char buf[STATUS_STRING_MAX];

		if(unlikely(*pfd < 0))	// open file first
		{
			*pfd = open(filename, O_RDONLY);
			if(unlikely(*pfd == -1))
			{
				return -(errno);
			}
		}
		else
		{
			lseek(*pfd, 0, SEEK_SET);	// rewind to start of file
		}

		// read from file
		if(unlikely(read(*pfd, buf, STATUS_STRING_MAX) == -1))
		{
			status =  -(errno);
		}
		else
		{
			status = atoi(buf);
		}
	}
#endif

	return status;
}

// ==============================================================================
// device status information getter functions
// ==============================================================================
static int get_wifi_status() 
{
	int wifi_status = 0;

#ifndef LOCALTEST
	#ifdef USE_VCONF
		if (unlikely(vconf_get_int(VCONFKEY_WIFI_STATE, &wifi_status) < 0))
		{
			wifi_status = VCONFKEY_WIFI_OFF;
		}
	#else
		if (unlikely(runtime_info_get_value_int(
					RUNTIME_INFO_KEY_WIFI_STATUS, &wifi_status) < 0))
		{
			wifi_status = RUNTIME_INFO_WIFI_STATUS_DISABLED;
		}
	#endif	// USE_VCONF
#endif

	return wifi_status;
}

static int get_bt_status()
{
	int bt_status = false;

#ifndef LOCALTEST
	#ifdef USE_VCONF
		if (unlikely(vconf_get_int(VCONFKEY_BT_STATUS, &bt_status) < 0))
		{
			bt_status = VCONFKEY_BT_STATUS_OFF;
		}
	#else
		if (unlikely(runtime_info_get_value_bool(
				RUNTIME_INFO_KEY_BLUETOOTH_ENABLED, (bool*)&bt_status) < 0))
		{
			bt_status = 0;
		}
	#endif	// USE_VCONF
#endif

	return bt_status;
}

static int get_gps_status()
{
	int gps_status = 0;

#ifndef LOCALTEST
	#ifdef USE_VCONF
		if(unlikely(vconf_get_bool(VCONFKEY_LOCATION_ENABLED, &gps_status) < 0))
		{
			gps_status = VCONFKEY_LOCATION_GPS_OFF;
		}
		else if(gps_status != 0)
		{
			if (unlikely(vconf_get_int(VCONFKEY_LOCATION_GPS_STATE, &gps_status) < 0))
			{
				gps_status = VCONFKEY_LOCATION_GPS_OFF;
			}
		}
	#else
		if (unlikely(runtime_info_get_value_bool(
					RUNTIME_INFO_KEY_LOCATION_SERVICE_ENABLED, (bool*)&gps_status) < 0))
		{
			gps_status = 0;
		}
		else if(gps_status != 0)
		{
			if (unlikely(runtime_info_get_value_bool(
					RUNTIME_INFO_KEY_LOCATION_ADVANCED_GPS_ENABLED, (bool*)&gps_status) < 0))
			{
				gps_status = 0;
			}
			else if(gps_status != 0)
			{
				if(unlikely(runtime_info_get_value_int(
								RUNTIME_INFO_KEY_GPS_STATUS, &gps_status) < 0))
				{
					gps_status = 0;
				}
				else
				{
					// do nothing (gps_status is real gps status)
				}
			}
			else
			{
				// do nothing (gps_status == 0)
			}
		}
		else
		{
			 // do nothing (gps_status == 0)
		}
	#endif	// USE_VCONF
#endif

	return gps_status;
}

static int get_brightness_status() 
{
	static int brightnessfd = -1;
	int brightness_status = 0;

#ifndef LOCALTEST
	#ifdef DEVICE_ONLY
		DIR* dir_info;
		struct dirent* dir_entry;
		char fullpath[PATH_MAX];

		dir_info = opendir(BRIGHTNESS_PARENT_DIR);
		if(dir_info != NULL)
		{
			while((dir_entry = readdir(dir_info)) != NULL)
			{
				if(strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
					continue;
				else	// first directory
				{
					sprintf(fullpath, BRIGHTNESS_PARENT_DIR "/%s/" BRIGHTNESS_FILENAME, dir_entry->d_name);
					brightness_status = get_file_status(&brightnessfd, fullpath);
				}
			}
		}
		else
		{
			// do nothing
		}
	#else
		brightness_status = get_file_status(&brightnessfd, EMUL_BRIGHTNESSFD);
	#endif
#endif	// LOCALTEST

	return brightness_status;
}

static int get_max_brightness() 
{
	int maxbrightnessfd = -1;
	static int max_brightness = -1;

#ifndef LOCALTEST
	if(__builtin_expect(max_brightness < 0, 0))
	{
	#ifdef DEVICE_ONLY
		DIR* dir_info;
		struct dirent* dir_entry;
		char fullpath[PATH_MAX];

		dir_info = opendir(BRIGHTNESS_PARENT_DIR);
		if(dir_info != NULL)
		{
			while((dir_entry = readdir(dir_info)) != NULL)
			{
				if(strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
					continue;
				else	// first directory
				{
					sprintf(fullpath, BRIGHTNESS_PARENT_DIR "/%s/" MAX_BRIGHTNESS_FILENAME, dir_entry->d_name);
					max_brightness = get_file_status(&maxbrightnessfd, fullpath);
				}
			}
		}
		else
		{
			// do nothing
		}
	#else
		max_brightness = get_file_status(&maxbrightnessfd, EMUL_MAX_BRIGHTNESSFD);
	#endif
	}

	if(maxbrightnessfd != -1)
		close(maxbrightnessfd);
#endif	// LOCALTEST

	return max_brightness;
}

static int get_video_status()
{
	static int videofd = -1;
	int video_status = 0;

#ifndef LOCALTEST
	video_status = get_file_status(&videofd, MFCFD);
	if (video_status > 1)
		video_status = 0;
#endif

	return video_status;
}

static int get_rssi_status()
{
#ifndef LOCALTEST
	int flightmode_status;

	#ifdef USE_VCONF
		int rssi_status;
		if(unlikely(vconf_get_bool(VCONFKEY_SETAPPL_FLIGHT_MODE_BOOL,
						&flightmode_status) < 0))
		{
			flightmode_status = 0;
		}

		if(!flightmode_status)
		{
			if(unlikely(vconf_get_int(VCONFKEY_TELEPHONY_RSSI, &rssi_status) < 0))
			{
				rssi_status = VCONFKEY_TELEPHONY_RSSI_0;
			}
		}
		else
		{
			rssi_status = VCONFKEY_TELEPHONY_RSSI_0;
		}

		return rssi_status;
	#else
		network_info_rssi_e rssi_status;

		if(unlikely(runtime_info_get_value_bool(
				RUNTIME_INFO_KEY_FLIGHT_MODE_ENABLED, (bool*)&flightmode_status) < 0))
		{
			flightmode_status = 0;
		}

		if(!flightmode_status)
		{
			if(unlikely(network_info_get_rssi(&rssi_status) < 0))
			{
				rssi_status = NETWORK_INFO_RSSI_0;
			}
		}
		else
		{
			rssi_status = NETWORK_INFO_RSSI_0;
		}

		return (int)rssi_status;
	#endif	// USE_VCONF
#else
	return 0;
#endif
}

static int get_call_status()
{
	int call_status = 0;

#ifndef LOCALTEST
	#ifdef USE_VCONF
		if(unlikely(vconf_get_int(VCONFKEY_CALL_STATE, &call_status) < 0))
		{
			call_status = VCONFKEY_CALL_OFF;
		}
	#else
		call_state_e call_state;
		if(unlikely(call_get_voice_call_state(&call_state) < 0))
		{
			if(unlikely(call_get_video_call_state(&call_state) < 0))
			{
				call_status = 0;	// call idle
			}
			else
			{
				// return video call state
				if(call_state == CALL_STATE_CONNECTING)
					call_status = 3;
				else if(call_state == CALL_STATE_ACTIVE)
					call_status = 4;
			}
		}
		else if(call_state == CALL_STATE_IDLE)	// voice call state is idle
		{
			if(unlikely(call_get_video_call_state(&call_state) < 0))
			{
				call_status = 0;
			}
			else
			{
				// return video call state
				if(call_state == CALL_STATE_CONNECTING)
					call_status = 3;
				else if(call_state == CALL_STATE_ACTIVE)
					call_status = 4;
			}
		}
		else
		{
			// return voice call state
			if(call_state == CALL_STATE_CONNECTING)
					call_status = 1;
			else if(call_state == CALL_STATE_ACTIVE)
					call_status = 2;
		}
	#endif	// USE_VCONF
#endif

	return call_status;
}

// dnet means 3G?
static int get_dnet_status()
{
	int dnet_status = false;

#ifndef LOCALTEST
	#ifdef USE_VCONF
		if(unlikely(vconf_get_int(VCONFKEY_DNET_STATE, &dnet_status) < 0))
		{
			dnet_status = VCONFKEY_DNET_OFF;
		}
	#else
		if(unlikely(runtime_info_get_value_bool(
				RUNTIME_INFO_KEY_PACKET_DATA_ENABLED, (bool*)&dnet_status) < 0))
		{
			dnet_status = 0;
		}
	#endif	// USE_VCONF
#endif

	return dnet_status;
}

static int get_camera_status()
{
	int camera_status = 0;

#ifndef LOCALTEST
#if 0
	#ifdef USE_VCONF
	if(unlikely(vconf_get_int(VCONFKEY_CAMERA_STATE, &camera_status) < 0))
	{
		camera_status = VCONFKEY_CAMERA_OFF;
	}
	#endif	// USE_VCONF
#endif
#endif

	return camera_status;
}

// this means silent mode?
static int get_sound_status() 
{
	int sound_status = 0;

#ifndef LOCALTEST
	#ifdef USE_VCONF
		if(unlikely(vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL,
						&sound_status) < 0))
		{
			sound_status = 0;
		}
	#else
		if(unlikely(runtime_info_get_value_bool(
				RUNTIME_INFO_KEY_SILENT_MODE_ENABLED, (bool*)&sound_status) < 0))
		{
			sound_status = 0;
		}
		else
		{
			sound_status = (sound_status == 1) ? 0 : 1;		// slient mode
		}
	#endif	// USE_VCONF
#endif

	return sound_status;
}

static int get_audio_status()
{
	int audio_state = 0;

#ifndef LOCALTEST

#ifdef DEVICE_ONLY
	int ret = 0;
	char dev[40];
	char state[3];
	FILE *fp = NULL;

	fp = fopen(AUDIOFD, "r");
	if(fp == NULL)
		return -1;

	while(ret != EOF) 
	{
		ret = fscanf(fp, "%[^:] %*c %[^\n] ", dev, state);
		if(strncmp(dev,"SPKR",4) == 0 && strncmp(state, "On",2) == 0)
		{
			audio_state = 1;
		} 
		else if(ret == 2 && strncmp(dev,"Head",4) == 0 && strncmp(state, "On",2) == 0) 
		{
			audio_state = 2;
			break;
		}
	}

	fclose(fp);
#endif

#endif	// LOCALTEST

	return audio_state;
}

static int get_vibration_status() 
{
	int vibration_status = 0;

#ifndef LOCALTEST
	#ifdef USE_VCONF
		if(unlikely(vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL,
						&vibration_status) < 0))
		{
			vibration_status = 0;
		}
	#else
		if(unlikely(runtime_info_get_value_bool(
					RUNTIME_INFO_KEY_VIBRATION_ENABLED, (bool*)&vibration_status) < 0))
		{
			vibration_status = 0;
		}
	#endif	// USE_VCONF
#endif

	return vibration_status;
}

static int get_voltage_status()
{
	static int voltagefd = -2;
	return get_file_status(&voltagefd, VOLTAGEFD);
}

// =====================================================================
// cpu information getter functions
// =====================================================================
static int get_cpu_frequency()
{
	static int frequencyfd = -2;
	int ret = -1;

	if(frequencyfd != -1)	// first time or file is exist
		ret = get_file_status(&frequencyfd, FREQFD);

	if(ret < 0)		// error occured
	{
		FILE* cpufp;
		char buf[SMALL_BUFFER];
		int strsize = strlen(CPUMHZ);
		double freq;

		cpufp = fopen(PROCCPUINFO, "r");
		if(likely(cpufp != NULL))
		{
			while(fgets(buf, SMALL_BUFFER, cpufp) != NULL)
			{
				if(strncmp(buf, CPUMHZ, strsize) == 0)
				{
					freq = atof(strchr(buf, ':') + 1);
					ret = (int)(freq * 1000.0);
					break;
				}
			}

			fclose(cpufp);
		}
		else
		{	// do nothing
		}
	}

	if(ret < 0)
		ret = 0;

	return ret;
}

// ========================================================================
// get cpu and memory info for each process and whole system
// ========================================================================
typedef struct _proc_node {
	proc_t proc_data;
	unsigned long long saved_utime;
	unsigned long long saved_stime;
	int found;
	struct _proc_node *next;
} procNode;

static procNode *prochead = NULL;
static procNode *thread_prochead = NULL;

static procNode* find_node(procNode *head, pid_t pid)
{
	procNode *t = head;

	while (t != NULL) {
		if (t->proc_data.pid == pid)
		{
			t->found = 1;
			break;
		}
		t = t->next;
	}
	return t;
}

static procNode* add_node(procNode **head, pid_t pid)
{
	procNode *n;

	n = (procNode *) malloc(sizeof(procNode));
	if (n == NULL) {
		LOGE("Not enough memory, add cpu info node failied");
		return NULL;
	}

	n->proc_data.pid = pid;
	n->found = 1;
	n->next = *head;
	*head = n;

	return n;
}

static int del_node(procNode **head, pid_t pid)
{
	procNode *t;
	procNode *prev;

	t = *head;
	prev = NULL;
	while (t != NULL) {
		if (t->proc_data.pid == pid) {
			if (prev != NULL)
				prev->next = t->next;
			else
				*head = (*head)->next;
			free(t);
			break;
		}
		prev = t;
		t = t->next;
	}

	return 0;
}

static int del_notfound_node(procNode **head)
{
	procNode *proc, *prev;
	prev = NULL;
	for(proc = *head; proc != NULL; )
	{
		if(proc->found == 0)
		{
			if(prev != NULL)
			{
				prev->next = proc->next;
				free(proc);
				proc = prev->next;
			}
			else
			{
				*head = (*head)->next;
				free(proc);
				proc = *head;
			}
		}
		else
		{
			prev = proc;
			proc = proc->next;
		}
	}
	return 0;
}

static int reset_found_node(procNode *head)
{
	procNode* proc;
	for(proc = head; proc != NULL; proc = proc->next)
	{
		proc->found = 0;
	}
	return 0;
}

// return 0 for normal case
// return negative value for error case
static int parse_proc_stat_file_bypid(char *path, proc_t* P)
{
	char filename[PROCPATH_MAX];
	char buf[BUFFER_MAX];
	int fd, num;
	char *abuf, *bbuf;

	// read from stat file
	sprintf(filename, "%s/stat", path);
	fd = open(filename, O_RDONLY, 0);

	if(unlikely(fd == -1))
		return -1;

	num = read(fd, buf, BUFFER_MAX);
	close(fd);

	if(unlikely(num <= 0))
		return -1;

	buf[num] = '\0';
	
	// scan from buffer
	// copy command name
	abuf = strchr(buf, '(') + 1;
	bbuf = strrchr(buf, ')');
	num = bbuf - abuf;
	if(unlikely(num >= sizeof(P->command)))
		num = sizeof(P->command) - 1;
	memcpy(P->command, abuf, num);
	P->command[num] = '\0';
	abuf = bbuf + 2;

	// scan data

	sscanf(abuf,
		"%c "
		"%d %d %d %d %d "
		"%lu %lu %lu %lu %lu "
		"%Lu %Lu %Lu %Lu "  // utime stime cutime cstime
		"%ld %ld "
		"%d "
		"%ld "
		"%Lu "  // start_time
		"%lu "
		"%ld",
		&P->state,
		&P->ppid, &P->pgrp, &P->sid, &P->tty_nr, &P->tty_pgrp,
		&P->flags, &P->minor_fault, &P->cminor_fault, &P->major_fault, &P->cmajor_fault,
		&P->utime, &P->stime, &P->cutime, &P->cstime,
		&P->priority, &P->nice,
		&P->numofthread,
		&P->dummy,
		&P->start_time,
		&P->vir_mem,
		&P->res_memblock
		);   

	if(P->numofthread == 0)
		P->numofthread = 1;

	return 0;
}

// return 0 for normal case
// return negative value for error case
static int parse_proc_smaps_file_bypid(char *path, proc_t* P)
{
#define MIN_SMAP_BLOCKLINE	50

	char filename[PROCPATH_MAX];
	char buf[MIDDLE_BUFFER];
	char numbuf[SMALL_BUFFER];
	FILE* fp;

	// reset pss size of proc_t
	P->pss = 0;

	// read from smaps file
	sprintf(filename, "%s/smaps", path);
	fp = fopen(filename, "r");

	if(fp == NULL)
		return -1;

	if(unlikely(probe_so_size == 0))	// probe so size is not abtained
	{
		int is_probe_so = 0;
		while(fgets(buf, MIDDLE_BUFFER, fp) != NULL)
		{
			if(strncmp(buf, "Pss:", 4) == 0)	// line is started with "Pss:"
			{
				sscanf(buf, "Pss:%s kB", numbuf);
				P->pss += atoi(numbuf);
				if(is_probe_so == 1)
				{
					probe_so_size += atoi(numbuf);
					is_probe_so = 0;	// reset search flag
				}
			}
			else	// not Pss line
			{
				if(is_probe_so == 0 && strlen(buf) > MIN_SMAP_BLOCKLINE)	// first we find probe so section
				{
					if(strstr(buf, DA_PROBE_TIZEN_SONAME) != NULL || 
							strstr(buf, DA_PROBE_OSP_SONAME) != NULL)	// found probe.so
					{
						is_probe_so = 1;
					}
					else
					{
						// do nothing
					}
				}
				else
				{
					// do nothing
				}
			}
		}
	}
	else	// we know about probe.so size already 
	{
		while(fgets(buf, MIDDLE_BUFFER, fp) != NULL)
		{
			if(strncmp(buf, "Pss:", 4) == 0)
			{
				sscanf(buf, "Pss:%s kB", numbuf);
				P->pss += atoi(numbuf);
			}
		}
	}

	P->pss -= probe_so_size;

	fclose(fp);

	return 0;
}

// return 0 for normal case
// return positive value for non critical case
// return negative value for critical case
static int update_process_data(int* pidarray, int pidcount, enum PROCESS_DATA datatype)
{
	static struct stat sb;
	int i, ret = 0;
	char buf[PROCPATH_MAX];
	procNode* procnode;

	for(i = 0; i < pidcount; i++)
	{
		if (pidarray[i] == 0)	// pid is invalid
		{
			ret = 1;
			continue;
		}

		sprintf(buf, "/proc/%d", pidarray[i]);
		if (unlikely(stat(buf, &sb) == -1))	// cannot access anymore
		{
			del_node(&prochead, pidarray[i]);
			ret = errno;
			continue;
		}

		if ((procnode = find_node(prochead, pidarray[i])) == NULL)	// new process
		{
			procnode = add_node(&prochead, pidarray[i]);
			if(datatype == PROCDATA_STAT)
			{
				if (unlikely((ret = parse_proc_stat_file_bypid(buf, &(procnode->proc_data))) < 0))
				{
					LOGE("Failed to get proc stat file by pid(%d)\n", pidarray[i]);
				}
				else
				{
					procnode->saved_utime = procnode->proc_data.utime;
					procnode->saved_stime = procnode->proc_data.stime;
				}
			}
			else if(datatype == PROCDATA_SMAPS)
			{
				if (unlikely((ret = parse_proc_smaps_file_bypid(buf, &(procnode->proc_data))) < 0))
				{
					LOGE("Failed to get proc smaps file by pid(%d)\n", pidarray[i]);
				}
				else
				{	// do nothing
				}
			}
			else
			{	// impossible
			}
		}
		else
		{
			if(datatype == PROCDATA_STAT)
			{
				if (unlikely((ret = parse_proc_stat_file_bypid(buf, &(procnode->proc_data))) < 0))
				{
					LOGE("Failed to get proc stat file by pid(%d)\n", pidarray[i]);
				}
				else
				{	// do nothing
				}
			}
			else if(datatype == PROCDATA_SMAPS)
			{
				if (unlikely((ret = parse_proc_smaps_file_bypid(buf, &(procnode->proc_data))) < 0))
				{
					LOGE("Failed to get proc smaps file by pid(%d)\n", pidarray[i]);
				}
				else
				{	// do nothing
				}
			}
			else
			{	// impossible
			}
		}
	}

	del_notfound_node(&prochead);
	reset_found_node(prochead);

	return ret;
}

static int update_system_cpu_frequency(int cur_index)
{
	char buf[SMALL_BUFFER];
	char filename[SMALL_BUFFER];
	int i, j;
	FILE* fp;

	// execute this block only once
	if(unlikely(num_of_freq <= 0))
	{
		FILE* fp;
		num_of_freq = 0;
		if((fp = fopen(CPUNUM_OF_FREQ, "r")) != NULL)
		{
			while(fgets(buf, SMALL_BUFFER, fp) != NULL)
			{
				num_of_freq++;
			}
			fclose(fp);
		}
		else
		{
			LOGW("Cannot open cpu0 frequency file\n");
			return -11;
		}

		for(i = 0; i < num_of_cpu; i++)
		{
			if(cpus[i].pfreq == NULL)
			{
				cpus[i].pfreq = (cpufreq_t*) calloc(num_of_freq, sizeof(cpufreq_t));
			}
		}
	}

	sprintf(filename, CPUNUM_OF_FREQ);
	// update cpu frequency information
	for(i = 0; i < num_of_cpu; i++)
	{
		filename[27] = (char)('0' + i);
		fp = fopen(filename, "r");
		if(fp != NULL)
		{
			for(j = 0; j < num_of_freq; j++)
			{
				if(fgets(buf, SMALL_BUFFER, fp) != NULL)
				{
					sscanf(buf, "%lu %Lu", &(cpus[i].pfreq[j].freq),
							&(cpus[i].pfreq[j].tick));
				}
				else	// cannot read anymore from frequency info file
					break;
			}

			fclose(fp);
			cpus[i].cur_freq_index = cur_index;
		}
		else	// cannot load cpu frequency information
		{	// do nothing
		}
	}

	return 0;
}

// return 0 for normal case
// return negative value for error
static int update_system_cpu_data(int cur_index)
{
	static FILE* fp = NULL;
	int num;
	char buf[BUFFER_MAX];

	if(fp == NULL)
	{
		if((fp = fopen(PROCSTAT, "r")) == NULL)
		{
			LOGE("Failed to open " PROCSTAT "\n");
			return -1;
		}
	}

	rewind(fp);
	fflush(fp);

	if(fgets(buf, sizeof(buf), fp) == NULL)
	{
		LOGE("Failed to read first line of " PROCSTAT "\n");
//		fclose(fp);
		return -1;
	}

	cpus[num_of_cpu].x = 0;
	cpus[num_of_cpu].y = 0;
	cpus[num_of_cpu].z = 0;
	num = sscanf(buf, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
			&cpus[num_of_cpu].u,
			&cpus[num_of_cpu].n,
			&cpus[num_of_cpu].s,
			&cpus[num_of_cpu].i,
			&cpus[num_of_cpu].w,
			&cpus[num_of_cpu].x,
			&cpus[num_of_cpu].y,
			&cpus[num_of_cpu].z
			);
	cpus[num_of_cpu].cur_load_index = cur_index;
	if(num < 4)
	{
		LOGE("Failed to read from " PROCSTAT "\n");
//		fclose(fp);
		return -1;
	}

#ifdef FOR_EACH_CPU
	// and just in case we're 2.2.xx compiled without SMP support...
	if(num_of_cpu == 1)
	{ 
		cpus[0].id = cpus[1].id = 0;
		memcpy(cpus, &cpus[1], sizeof(tic_t) * 8);
		cpus[0].cur_load_index = cur_index;
	}
	else if(num_of_cpu > 1)
	{
		int i;
		// now value each separate cpu's tics
		for(i = 0; i < num_of_cpu; i++)
		{
			if(fgets(buf, sizeof(buf), fp) != NULL)
			{
				cpus[i].x = 0;
				cpus[i].y = 0;
				cpus[i].z = 0;
				num = sscanf(buf, "cpu%u %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
						&cpus[i].id,
						&cpus[i].u, &cpus[i].n, &cpus[i].s, &cpus[i].i,
						&cpus[i].w, &cpus[i].x, &cpus[i].y, &cpus[i].z);
				if(num > 4)
				{
					LOGI("Readed %d stats of %dth cpu\n", num, i);
					cpus[i].cur_load_index = cur_index;
				}
				else	// buf is not cpu core tick information
				{	// do nothing
				}
			}
			else	// cannot read anymore from /proc/stat file
			{	// do nothing
			}
		}
	}
	else
	{
		// not possible
//		fclose(fp);
		return -1;
	}
#endif

//	fclose(fp);
	return 0;
}

// return 0 for normal case
// return negative value for error
static int update_system_memory_data(unsigned long *memtotal, unsigned long *memused)
{
	static int meminfo_fd = -1;
	char *head, *tail;
	int i, num;
	char buf[BUFFER_MAX];
	static const mem_t mem_table[] = {
		{"Buffers",		&mem_slot_array[MEM_SLOT_BUFFER]},
		{"Cached",		&mem_slot_array[MEM_SLOT_CACHED]},
		{"MemFree",		&mem_slot_array[MEM_SLOT_FREE]},
		{"MemTotal",	&mem_slot_array[MEM_SLOT_TOTAL]},
	};
	const int mem_table_size = sizeof(mem_table) / sizeof(mem_t);

	if(meminfo_fd == -1 && (meminfo_fd = open(PROCMEMINFO, O_RDONLY)) == -1)
	{
		LOGE("Failed to open " PROCMEMINFO "\n");
		return -1;
	}
	lseek(meminfo_fd, 0L, SEEK_SET);
	if((num = read(meminfo_fd, buf, BUFFER_MAX)) < 0)
	{
		LOGE("Failed to read from " PROCMEMINFO "\n");
		return -1;
	}
	buf[num] = '\0';

	num = 0;	// number of found element
	head = buf;
	for( ;num < mem_table_size ; )
	{
		tail = strchr(head, ':');
		if(!tail)
			break;
		*tail = '\0';
		for(i = 0; i < mem_table_size; i++)
		{
			if(strcmp(head, mem_table[i].name) == 0)	// found
			{
				head = tail + 1;
				*(mem_table[i].slot) = strtoul(head, &tail, 10);
				num++;
				break;
			}
		}
		if(i == mem_table_size)	// cannot find entry
			head = tail + 1;
		tail = strchr(head, '\n');
		if(tail == NULL)
			break;
		head = tail + 1;
	}

	if(num == mem_table_size)	// find all element
	{
		*memtotal = mem_slot_array[MEM_SLOT_TOTAL];
		*memused = mem_slot_array[MEM_SLOT_TOTAL] - mem_slot_array[MEM_SLOT_FREE] -
			mem_slot_array[MEM_SLOT_BUFFER] - mem_slot_array[MEM_SLOT_CACHED];

		*memtotal *= 1024;	// change to Byte
		*memused *= 1024;	// change to Byte
		return 0;
	}
	else
	{
		LOGE("Cannot find all neccessary element in meminfo\n");
		return -1;
	}
}

// return 0 for error case
// return system total memory in MB
static unsigned long get_system_total_memory()
{
	int meminfo_fd = -1;
	char *head, *tail;
	int num;
	char buf[BUFFER_MAX];
	static const char* memtotalstr = "MemTotal";
	unsigned long totalmem = 0;

	if((meminfo_fd = open(PROCMEMINFO, O_RDONLY)) == -1)
	{
		LOGE("Failed to open " PROCMEMINFO "\n");
		return 0;
	}
	if((num = read(meminfo_fd, buf, BUFFER_MAX)) < 0)
	{
		LOGE("Failed to read from " PROCMEMINFO "\n");
		return 0;
	}
	buf[num] = '\0';
	close(meminfo_fd);

	head = buf;
	for( ; ; )
	{
		tail = strchr(head, ':');
		if(!tail)
			break;
		*tail = '\0';
		if(strcmp(head, memtotalstr) == 0)	// found
		{
			head = tail + 1;
			totalmem = strtoul(head, &tail, 10);
			break;
		}

		head = tail + 1;
		tail = strchr(head, '\n');
		if(tail == NULL)
			break;
		head = tail + 1;
	}
	
	return (totalmem * 1024);
}

// ===============================================================
// disk information getter functions
// ===============================================================
static int get_fsinfo(const char* path, int type)
{
	struct statfs buf;
	int total;
	int free;

	if (statfs(path, &buf) < 0)
	{
		return -errno;
	}

	total = (int)((long long)(buf.f_bsize / 1024LL * buf.f_blocks) / 1024LL);
	free = (int)((long long)(buf.f_bsize / 1024LL * buf.f_bavail) / 1024LL);

	LOGI("File storage total(%d), free(%d)\n", total, free);
	if (type == FSINFO_TYPE_TOTAL)
	{
		return total;
	}
	else if (type == FSINFO_TYPE_FREE)
	{
		return free;
	}

	return -1;
}

static int stat_get_storageinfo(int type)
{
	return get_fsinfo(UMSFD, type);
}

static int stat_get_cardinfo(int type)
{
	if (access(MMCBLKFD, F_OK) < 0)
	{
		return -1;
	}

	return get_fsinfo(MMCFD, type);
}


static int get_total_drive()
{
	int total = 0;
	int storage = stat_get_storageinfo(FSINFO_TYPE_TOTAL);
	int card = stat_get_cardinfo(FSINFO_TYPE_TOTAL);

	if (storage < 0 && card < 0)
	{
		return -1;
	}

	total = storage + card;

	return total;
}

static int get_total_used_drive()
{
	int total = 0;
	int free = 0;
	int storage = stat_get_storageinfo(FSINFO_TYPE_FREE); 
	int card = stat_get_cardinfo(FSINFO_TYPE_FREE);

	if (storage < 0 && card < 0)
	{
		return -1;
	}

	free = storage + card;
	total = get_total_drive() - free;

	return total;
}

static int update_thread_data(int pid)
{
	static struct stat sb;
	int ret = 0;
	char path[PROCPATH_MAX];
	char buf[PROCPATH_MAX];
	procNode* procnode;
	DIR *taskdir = NULL;
	struct dirent *entry = NULL;
	unsigned int tid;

	sprintf(path, "/proc/%d/task", pid);

	if(!(taskdir = opendir(path)))
	{
		return -1;
	}

	while((entry = readdir(taskdir)) != NULL)
	{
		if(*entry->d_name > '0' &&	*entry->d_name <= '9')
		{	
			tid = atoi(entry->d_name);
			sprintf(buf, "/proc/%d/task/%d", pid, tid);

			if(unlikely(stat(buf, &sb) == -1))
			{
				del_node(&thread_prochead, tid);
				ret = errno;
				continue;
			}

			if((procnode = find_node(thread_prochead, tid)) == NULL)
			{
				procnode = add_node(&thread_prochead, tid);
				if(unlikely((ret = parse_proc_stat_file_bypid(buf, &(procnode->proc_data))) < 0))
				{
					LOGE("Failed to get proc stat file by tid(%d)\n", tid);
				}
				else
				{
					procnode->saved_utime = procnode->proc_data.utime;
					procnode->saved_stime = procnode->proc_data.stime;
				}
			}
			else
			{
				if(unlikely((ret = parse_proc_stat_file_bypid(buf, &(procnode->proc_data))) < 0))
				{
					LOGE("Failed to get proc stat file by tid(%d)\n", tid);
				}
			}
		}
	}

	del_notfound_node(&thread_prochead);
	reset_found_node(thread_prochead);

	closedir(taskdir);
	return ret;
}

// ========================================================================
// overall information getter functions
// ========================================================================

// this code is not necessary anymore
/*
static void get_app_info(const char* binary_path, char* width, 
		char* height, char* theme, char* version, 
		char* scale, char* removable, 
		char* comment)
{
	int fd = 0;
	int res = 0;
	int i = 0;
	int j = 0;
	char pkg_info_path [PATH_MAX];
	char buffer [BUFFER_MAX];

	sprintf(pkg_info_path, "/opt/share/applications/%s.desktop", pkg_name);

    fd = open(pkg_info_path, O_RDONLY);
    if (fd < 0)
	{
		LOGE("Cannot open %s", pkg_info_path);
		return;
	}

	fcntl( fd, F_SETFD, FD_CLOEXEC );

	LOGI("get_app_info - After open pkg_info_path\n");

	for (;;)
	{
		res = read(fd, buffer, BUFFER_MAX);
		if (res == 0)
		{
			break;
		} 
		if (res < 0) 
		{
			if (errno == EINTR)
				continue;
			else
				break;
		}

		LOGI("read buffer ===%s===\n", buffer);
		for (i = 0; i < res; i++) 
		{
			if (i < res - 22 && strncmp(&buffer[i], "X-SLP-BaseLayoutWidth=", 22) == 0)
			{
				for (j = 0; j < res; j ++)
				{
					if (buffer[i+j] == '\n' || buffer[i+j] == '\t')
					{
						LOGI("width :::: ");
						strncpy(width, &(buffer[i+22]), j-22);
						LOGI("%s\n", width);
						break;
					}
				}
				i = i + j;
			}
			else if (i < res - 23 && strncmp(&buffer[i], "X-SLP-BaseLayoutHeight=", 23) == 0)
			{
				for (j = 0; j < res; j ++)
				{
					if (buffer[i+j] == '\n' || buffer[i+j] == '\t')
					{
						LOGI("height :::: ");
						strncpy(height, &(buffer[i+23]), j-23);
						LOGI("%s\n", height);
						break;
					}
				}
				i = i + j;
			}
			else if (i < res - 6 && (strncmp(&buffer[i], "theme=", 6) == 0 || strncmp(&buffer[i], "Theme=", 6) == 0))
			{
				for (j = 0; j < res; j ++)
				{
					if (buffer[i+j] == '\n' || buffer[i+j] == '\t')
					{
						LOGI("theme :::: ");
						strncpy(theme, &(buffer[i+6]), j-6);
						LOGI("%s\n", theme);
						break;
					}
				}
				i = i + j;
			}
			else if (i < res - 8 && (strncmp(&buffer[i], "Version=", 8) == 0 || strncmp(&buffer[i], "version=", 8) == 0))
			{
				for (j = 0; j < res; j ++)
				{
					if (buffer[i+j] == '\n' || buffer[i+j] == '\t')
					{
						LOGI("version :::: ");
						strncpy(version, &(buffer[i+8]), j-8);
						LOGI("%s\n", version);
						break;
					}
				}
				i = i + j;
			}
			else if (i < res - 24 && strncmp(&buffer[i], "X-SLP-IsHorizontalScale=", 24) == 0)
			{
				for (j = 0; j < res; j ++)
				{
					if (buffer[i+j] == '\n' || buffer[i+j] == '\t')
					{
						LOGI("scale :::: ");
						strncpy(scale, &(buffer[i+24]), j-24);
						LOGI("%s\n", scale);
						break;
					}
				}
				i = i + j;
			}
			else if (i < res - 16 && strncmp(&buffer[i], "X-SLP-Removable=", 16) == 0)
			{
				for (j = 0; j < res; j ++)
				{
					if (buffer[i+j] == '\n' || buffer[i+j] == '\t')
					{
						LOGI("removable :::: ");
						strncpy(removable, &(buffer[i+16]), j-16);
						LOGI("%s\n", removable);
						break;
					}
				}
				i = i + j;
			}
			else if (i < res - 8 && (strncmp(&buffer[i], "Comment=", 8) == 0 || strncmp(&buffer[i], "comment=", 8) == 0))
			{
				for (j = 0; j < res; j ++)
				{
					if (buffer[i+j] == '\n' || buffer[i+j] == '\t')
					{
						LOGI("comments :::: ");
						strncpy(comment, &(buffer[i+8]), j-8);
						LOGI("%s\n", comment);
						break;
					}
				}
				i = i + j;
			}
		}
	}

	close(fd);
}
*/

static int get_device_availability_info(char* buf, int buflen)
{
	int camera_count = 0;
	bool blue_support = false;
	bool gps_support = false;
	bool wifi_support = false;
	char* networktype = NULL;
	int loglen = 0;

#ifndef LOCALTEST
	system_info_get_value_bool(SYSTEM_INFO_KEY_BLUETOOTH_SUPPORTED, &blue_support);
	system_info_get_value_int(SYSTEM_INFO_KEY_CAMERA_COUNT, &camera_count);
	system_info_get_value_bool(SYSTEM_INFO_KEY_GPS_SUPPORTED, &gps_support);
	system_info_get_value_string(SYSTEM_INFO_KEY_NETWORK_TYPE, &networktype);
	system_info_get_value_bool(SYSTEM_INFO_KEY_WIFI_SUPPORTED, &wifi_support);
#endif

	loglen += sprintf(buf, "%d`,%d`,%d`,%d`,",
			(int)blue_support,
			(int)gps_support,
			(int)wifi_support,
			camera_count);

	if(networktype != NULL)
	{
		loglen += sprintf(buf + loglen, "%s", networktype);
		free(networktype);
	}
	else
	{
		// do nothing
	}

	return loglen;
}

int get_device_info(char* buffer, int buffer_len)
{
	int res = 0;
/*
	char width[BUFFER_MAX];
	char height[BUFFER_MAX];
	char theme[BUFFER_MAX];
	char version[BUFFER_MAX];
	char scale[BUFFER_MAX];
	char removable[BUFFER_MAX];
	char comment[BUFFER_MAX * 2];

	memset(width, 0, sizeof(width));
	memset(height, 0, sizeof(height));
	memset(theme, 0, sizeof(theme));
	memset(version, 0, sizeof(version));
	memset(scale, 0, sizeof(scale));
	memset(removable, 0, sizeof(removable));
	memset(comment, 0, sizeof(comment));
*/
	res += sprintf(buffer, "%lu`,%d`,", get_system_total_memory(), get_total_drive());
	res += get_device_availability_info(buffer + res, buffer_len - res);
	res += sprintf(buffer + res, "`,%d", get_max_brightness());

	res += sprintf(buffer + res, "`,`,`,`,`,`,`,");
//	res += sprintf(buffer + res, "`,%s`,%s`,%s`,%s`,%s`,%s`,%s", width, height, theme, version, scale, removable, comment);

	return res;
}

// return log length (>0) for normal case
// return negative value for error
int get_resource_info(char* buffer, int buffer_len, int* pidarray, int pidcount)
{
	static struct timeval old_time = {0, 0};
	static int event_num = 0;

	int res = 0;
	int i, j;
	float elapsed;
	float factor;
	procNode* proc;
	CPU_t* cpuptr;
	struct timeval current_time;

	// data variable
	unsigned long virtual = 0;
	unsigned long resident = 0;
	unsigned long shared = 0;
	unsigned long pssmem = 0;
	int num_thread = 0;
	unsigned long long ticks = 0, freqsum;
	float app_cpu_usage, sys_usage, thread_load;
	unsigned long sysmemtotal = 0;
	unsigned long sysmemused = 0;
	unsigned long curtime;
//	long long idle_tick_sum = 0, total_tick_sum = 0;
	int call_time = 0;
	int rssi_time = 0;

	// buffers
	char thread_loadbuf[LARGE_BUFFER] = {0, };
	char thread_loadtmpbuf[SMALL_BUFFER];
	char freqbuf[MIDDLE_BUFFER];
	char cpuload[SMALL_BUFFER];
	int freqbufpos = 0;
	int cpuloadpos = 0;
//	unsigned int failed_cpu = 0;

	LOGI("PID count : %d\n", pidcount);

	if(update_process_data(pidarray, pidcount, PROCDATA_STAT) < 0)
	{
		LOGE("Failed to update process stat data\n");
		return -1;
	}

	// this position is optimized position of timestamp.
	// just before get system cpu data and just after get process cpu data
	// because cpu data is changed so fast and variance is so remarkable
	gettimeofday(&current_time, NULL);	// DO NOT MOVE THIS SENTENCE!

	if(update_system_cpu_data(event_num) < 0)
	{
		LOGE("Failed to update system cpu data\n");
		return -1;
	}

	// freqbufpos is temporary used
	freqbufpos = update_system_cpu_frequency(event_num);

	// memory data is changed slowly and variance is not remarkable
	// so memory data is less related with timestamp then cpu data
	if(update_process_data(pidarray, pidcount, PROCDATA_SMAPS) < 0)
	{
		LOGE("Failed to update process smaps data\n");
		return -1;
	}

	if(update_thread_data(pidarray[0]) < 0)
	{
		LOGE("Failed to update thread stat data\n");
		return -1;
	}

	if(update_system_memory_data(&sysmemtotal, &sysmemused) < 0)
	{
		LOGE("Failed to update system memory data\n");
		return -1;
	}

	// prepare calculate
	elapsed = (current_time.tv_sec - old_time.tv_sec) +
		((float)(current_time.tv_usec - old_time.tv_usec) / 1000000.0f);
	old_time.tv_sec = current_time.tv_sec;
	old_time.tv_usec = current_time.tv_usec;
	curtime = ((unsigned long)current_time.tv_sec * 10000) +
			((unsigned long)current_time.tv_usec / 100);

	factor = 100.0f / ((float)Hertz * elapsed * num_of_cpu);


	// calculate process for process
	for(proc = prochead; proc != NULL; proc = proc->next)
	{
		num_thread += proc->proc_data.numofthread;
		virtual += proc->proc_data.vir_mem;					// Byte
		resident += (proc->proc_data.res_memblock * 4);		// KByte
		pssmem += (proc->proc_data.pss);					// KByte
		ticks += (proc->proc_data.utime + proc->proc_data.stime - proc->saved_utime - proc->saved_stime);

		proc->saved_utime = proc->proc_data.utime;
		proc->saved_stime = proc->proc_data.stime;
	}
	app_cpu_usage = (float)ticks * factor;
	if(app_cpu_usage > 100.0f)
		app_cpu_usage = 100.0f;
	resident = resident * 1024;		// change to Byte
	pssmem = pssmem * 1024;			// change to Byte

	// calculate thread load
	for(proc = thread_prochead; proc != NULL; proc = proc->next)
	{
		thread_load = (float)(proc->proc_data.utime + proc->proc_data.stime - proc->saved_utime - proc->saved_stime)
			* factor;
		if(thread_load > 100.0f)
			thread_load = 100.0f;

		sprintf(thread_loadtmpbuf, "%d,%.1f,", proc->proc_data.pid, thread_load);
		strcat(thread_loadbuf, thread_loadtmpbuf);

		proc->saved_utime = proc->proc_data.utime;
		proc->saved_stime = proc->proc_data.stime;
	}

	// calculate for system cpu load
#ifdef FOR_EACH_CPU
	for(i = 0; i < num_of_cpu; i++)
#else
	for(i = num_of_cpu; i <= num_of_cpu; i++)
#endif
	{
		cpuptr = &(cpus[i]);

		if(cpuptr->cur_load_index == event_num)
		{
			if(cpuptr->sav_load_index == event_num - 1)		// previous sampling is just before 1 period
			{
				cpuptr->idle_ticks = cpuptr->i - cpuptr->i_sav;
				if(unlikely(cpuptr->idle_ticks < 0))
				{
					cpuptr->idle_ticks = 0;
				}
				cpuptr->total_ticks = (cpuptr->u - cpuptr->u_sav) +
					(cpuptr->s - cpuptr->s_sav) +
					(cpuptr->n - cpuptr->n_sav) +
					cpuptr->idle_ticks +
					(cpuptr->w - cpuptr->w_sav) +
					(cpuptr->x - cpuptr->x_sav) +
					(cpuptr->y - cpuptr->y_sav) +
					(cpuptr->z - cpuptr->z_sav);
				if(cpuptr->total_ticks < MIN_TOTAL_TICK)
				{
					cpuptr->cpu_usage = 0.0f;
				}
				else
				{
					cpuptr->cpu_usage = (1.0f - ((float)cpuptr->idle_ticks / (float)cpuptr->total_ticks)) * 100.0f;
				}
//				if(i != num_of_cpu)
//				{
//					idle_tick_sum += cpuptr->idle_ticks;
//					total_tick_sum += cpuptr->total_ticks;
//				}
				LOGI("System cpu usage log : %d, %Ld, %Ld\n", i, cpuptr->idle_ticks, cpuptr->total_ticks);
				if(unlikely(cpuptr->cpu_usage < 0))
				{
					cpuptr->cpu_usage = 0.0f;
				}
			}
			else	// previous sampling is not just before 1 period
			{
				// assume non idle ticks happen in 1 profiling period
				// because sampling is not available just before 1 profiling period
				cpuptr->idle_ticks = (cpuptr->u - cpuptr->u_sav) +
					(cpuptr->s - cpuptr->s_sav) +
					(cpuptr->n - cpuptr->n_sav) +
					(cpuptr->w - cpuptr->w_sav) +
					(cpuptr->x - cpuptr->x_sav) +
					(cpuptr->y - cpuptr->y_sav) +
					(cpuptr->z - cpuptr->z_sav);
				cpuptr->total_ticks = (long long)(Hertz * elapsed);
				if(unlikely(cpuptr->total_ticks < 1))
					cpuptr->total_ticks = 1;
				cpuptr->cpu_usage = ((float)cpuptr->idle_ticks / (float)cpuptr->total_ticks) * 100.0f;
				if(unlikely(cpuptr->cpu_usage > 100.0f))
				{
					cpuptr->cpu_usage = 100.0f;
				}
			}

			// save new value
			cpuptr->u_sav = cpuptr->u;
			cpuptr->s_sav = cpuptr->s;
			cpuptr->n_sav = cpuptr->n;
			cpuptr->i_sav = cpuptr->i;
			cpuptr->w_sav = cpuptr->w;
			cpuptr->x_sav = cpuptr->x;
			cpuptr->y_sav = cpuptr->y;
			cpuptr->z_sav = cpuptr->z;
			cpuptr->sav_load_index = cpuptr->cur_load_index;
		}
		else
		{
			cpuptr->cpu_usage = 0.0f;
		}
	}

#ifdef FOR_EACH_CPU
	// calculate for cpu core load that failed to get tick information
/*
	if(failed_cpu != 0)
	{
		LOGI("ticks1 : %Ld, %Ld\n", idle_tick_sum, total_tick_sum);
		idle_tick_sum = cpus[num_of_cpu].idle_ticks - idle_tick_sum;
		total_tick_sum = cpus[num_of_cpu].total_ticks - total_tick_sum;
		LOGI("ticks2 : %Ld, %Ld\n", idle_tick_sum, total_tick_sum);
		if(total_tick_sum >= MIN_TICKS_FOR_LOAD)
			sys_usage = (1.0f - ((float)idle_tick_sum / (float)total_tick_sum)) * 100.0f;
		else
			sys_usage = 0.0f;
		if(sys_usage < 0.0f) sys_usage = 0.0f;
		else if(sys_usage > 100.0f) sys_usage = 100.0f;

		for(i = 0; i < num_of_cpu; i++)
		{
			if(failed_cpu & (1 << i))
			{
				cpus[i].cpu_usage = sys_usage;
			}
		}
	}
*/

	// calculate for whole cpu load by average all core load
	sys_usage = 0.0f;
	for(i = 0 ; i < num_of_cpu; i++)
	{
		sys_usage += cpus[i].cpu_usage;
	}
	cpus[num_of_cpu].cpu_usage = sys_usage / num_of_cpu;

	// make cpu load string
	for(i = 0; i <= num_of_cpu; i++)
	{
		cpuloadpos += sprintf(cpuload + cpuloadpos, "%.1f,", cpus[i].cpu_usage);
	}
	cpuload[cpuloadpos - 1] = '\0';		// remove last ,
#else
	sprintf(cpuload, "%.1f", cpus[num_of_cpu].cpu_usage);
#endif

	// calculate for system cpu frequency
	if(freqbufpos >= 0)
	{
		sys_usage = 0.0f;
		freqbufpos = 0;
		ticks = 0;
		freqsum = 0;
		for(i = 0; i < num_of_cpu; i++)
		{
			cpuptr = &(cpus[i]);

			if(cpuptr->cur_freq_index == event_num)
			{
				if(cpuptr->sav_freq_index == event_num - 1)
				{
					for(j = 0; j < num_of_freq; j++)
					{
						freqsum += (cpuptr->pfreq[j].freq *
								(cpuptr->pfreq[j].tick - cpuptr->pfreq[j].tick_sav));
						ticks += (cpuptr->pfreq[j].tick - cpuptr->pfreq[j].tick_sav);
					}
				}
				else
				{	// do nothing
				}

				for(j = 0; j < num_of_freq; j++)
				{
					cpuptr->pfreq[j].tick_sav = cpuptr->pfreq[j].tick;		// restore last tick value
				}
				cpuptr->sav_freq_index = cpuptr->cur_freq_index;
			}

#ifdef FOR_EACH_CPU
			if(ticks != 0)
			{
				if(sys_usage == 0.0f)
					sys_usage = (float)freqsum / (float)ticks;
				freqbufpos += sprintf(freqbuf + freqbufpos, "%.0f,", (float)freqsum / (float)ticks);
			}
			else
			{
				freqbufpos += sprintf(freqbuf + freqbufpos, "%.0f,", sys_usage);
			}
			ticks = 0;
			freqsum = 0;
#endif
		}
#ifndef FOR_EACH_CPU
		freqbufpos += sprintf(freqbuf + freqbufpos, "%.0f", (float)freqsum / (float)ticks);
#else
		freqbuf[freqbufpos - 1] = '\0';		// remove last ,
#endif
	}
	else	// update system cpu frequency is failed
	{
		freqbufpos = get_cpu_frequency();
		sprintf(freqbuf, "%d", (freqbufpos > 0)? freqbufpos : 0);
	}

	// print log
	res = sprintf(buffer, "9`,"
			"%d`,%d`,%lu`,"				// event number, reserved, current time
			"%d`,%d`,%d`,"				// device status(wifi, bt, gps)
			"%d`,%d`,%d`,"				// device status(bright, camera, sound)
			"%d`,%d`,%d`,"				// device status(audio, vibration, voltage)
			"%d`,%d`,%d`,%d`,"			// device status(rssi, video, call, dnet status)
			"%d`,%d`,"					// device status(call_time, rssi_time)
			"%s`,%.1f`,%s`,"			// cpu frequency and usage (app, system)
			"%lu`,%lu`,%lu`,%lu`,%Ld`,"	// process memory
			"%lu`,%lu`,"				// system total memory, used memory
			"%d`,"						// system drive
			"%d`,"						// number of thread
			"%s`,\n",					// thread load
			event_num, 0, curtime,		// 0 is reserved value
			get_wifi_status(), get_bt_status(),	get_gps_status(),
			get_brightness_status(), get_camera_status(), get_sound_status(),
			get_audio_status(), get_vibration_status(), get_voltage_status(),
			get_rssi_status(), get_video_status(), get_call_status(), get_dnet_status(),
			call_time, rssi_time,
			freqbuf, app_cpu_usage, cpuload,
			virtual, resident, shared, pssmem, get_total_alloc_size(),
			sysmemtotal, sysmemused,
			get_total_used_drive(),
			num_thread,
			thread_loadbuf
			);
	
	LOGI("get_resource_info result : %s", buffer);

	event_num++;
	return res;
}

int initialize_system_info()
{
	int i;

	num_of_cpu = sysconf(_SC_NPROCESSORS_ONLN);
	if(num_of_cpu < 1)
		num_of_cpu = 1;
	Hertz = sysconf(_SC_CLK_TCK);
	LOGI("Hertz : %d\n", Hertz);

	// alloc for cpus
	if(cpus == NULL)
		cpus = (CPU_t*) calloc((num_of_cpu + 1), sizeof(CPU_t));
	if(cpus != NULL)
	{
		for(i = 0; i <= num_of_cpu; i++)
		{
			cpus[i].cur_load_index = cpus[i].sav_load_index = -1;
			cpus[i].cur_freq_index = cpus[i].sav_freq_index = -1;
		}
	}
	else
	{
		LOGE("Failed to alloc memory for cpu information\n");
		return -1;
	}

	return 0;
}

int finalize_system_info()
{
	int i;

	if(cpus != NULL)
	{
		for(i = 0; i < num_of_cpu; i++)
		{
			if(cpus[i].pfreq != NULL)
				free(cpus[i].pfreq);
		}

		free(cpus);
	}

	return 0;
}

