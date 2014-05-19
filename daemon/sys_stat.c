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

#include <glob.h>
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
#include <errno.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>

#include "da_protocol.h"
#include "da_data.h"
#include "sys_stat.h"
#include "daemon.h"
#include "device_system_info.h"
#include "device_vconf.h"
#include "device_camera.h"
#include "debug.h"

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

// define for correct difference of system feature vars
#define val_diff(v_new, v_old) ((v_new < v_old) ? v_new : v_new - v_old)

enum PROCESS_DATA
{
	PROCDATA_STAT,
	PROCDATA_SMAPS
};

// declared by greatim
static int Hertz = 0;
static int num_of_cpu = 0;
static int num_of_freq = 0;
static uint64_t mem_slot_array[MEM_SLOT_MAX];
static CPU_t* cpus = NULL;
static unsigned long probe_so_size = 0;


static int get_file_status_no_open(int pfd, const char *filename)
{
	int status = 0;
	char buf[STATUS_STRING_MAX];

	if (unlikely(pfd < 0)) {
		// file is not open
		return 0;
	}

	lseek(pfd, 0, SEEK_SET);	// rewind to start of file

	// read from file
	if (unlikely(read(pfd, buf, STATUS_STRING_MAX) == -1))
		status =  -(errno);
	else
		status = atoi(buf);

	return status;
}
// daemon api : get status from file
// pfd must not be null
int get_file_status(int *pfd, const char *filename)
{
	int status = 0;

	if (likely(pfd != NULL)) {
		//open if is not open
		if (unlikely(*pfd < 0)) {
			// open file first
			*pfd = open(filename, O_RDONLY);
			if (unlikely(*pfd == -1)) {
				/* This file may absent in the system */
				return 0;
			}
		}

		if (unlikely(*pfd < 0)) {
			//file is open. lets read
			status = get_file_status_no_open(*pfd, filename);
		}

	}

	return status;
}

// =============================================================================
// device status information getter functions
// =============================================================================

static void init_brightness_status()
{
#ifdef DEVICE_ONLY
	DIR *dir_info;
	struct dirent *dir_entry;
	char fullpath[PATH_MAX];

	dir_info = opendir(BRIGHTNESS_PARENT_DIR);
	if (dir_info != NULL) {
		while ((dir_entry = readdir(dir_info)) != NULL) {
			if (strcmp(dir_entry->d_name, ".") == 0 ||
			    strcmp(dir_entry->d_name, "..") == 0)
				continue;
			else { /* first directory */
				sprintf(fullpath,
					BRIGHTNESS_PARENT_DIR "/%s/"
					BRIGHTNESS_FILENAME,
					dir_entry->d_name);
				get_file_status(&manager.fd.brightness,
						fullpath);
			}
		}
		closedir(dir_info);
	} else {
		/* do nothing */
	}
#else
	get_file_status(&manager.fd.brightness, EMUL_BRIGHTNESSFD);
#endif
}

static int get_brightness_status()
{
	return get_file_status_no_open(manager.fd.brightness, EMUL_BRIGHTNESSFD);
}

static int get_max_brightness()
{
	int maxbrightnessfd = -1;
	static int max_brightness = -1;

	if (__builtin_expect(max_brightness < 0, 0)) {
#ifdef DEVICE_ONLY
		DIR* dir_info;
		struct dirent* dir_entry;
		char fullpath[PATH_MAX];

		dir_info = opendir(BRIGHTNESS_PARENT_DIR);
		if (dir_info != NULL) {
			while((dir_entry = readdir(dir_info)) != NULL) {
				if (strcmp(dir_entry->d_name, ".") == 0 ||
				    strcmp(dir_entry->d_name, "..") == 0)
					continue;
				else { /* first */
					sprintf(fullpath,
						BRIGHTNESS_PARENT_DIR "/%s/" MAX_BRIGHTNESS_FILENAME,
						dir_entry->d_name);
					max_brightness = get_file_status(&maxbrightnessfd, fullpath);
				}
			}
			closedir(dir_info);
		} else {
			// do nothing
		}
#else /* DEVICE_ONLY */
		max_brightness = get_file_status(&maxbrightnessfd, EMUL_MAX_BRIGHTNESSFD);
#endif /* DEVICE_ONLY */
	}

	if (maxbrightnessfd != -1)
		close(maxbrightnessfd);

	return max_brightness;
}

static void init_video_status()
{
	manager.fd.video = fopen(MFCFD, "r");
}

static int get_video_status()
{
	int video_status = 0;
	int ret;
	FILE *video_fp = manager.fd.video;
	char stat[256];

	if (video_fp == NULL) // file is not open
		return 0;

	rewind(video_fp);
	fflush(video_fp);

	ret = fscanf(video_fp, "%s", stat);

	if (ret != EOF)
		if(strncmp(stat,"active",6) == 0)
			video_status = 1;

	return video_status;
}

static void init_voltage_status()
{
	get_file_status(&manager.fd.voltage, VOLTAGEFD);
}

static int get_voltage_status()
{
	return get_file_status_no_open(manager.fd.voltage, VOLTAGEFD);
}

// =====================================================================
// cpu information getter functions
// =====================================================================
static void get_cpu_frequency(float *freqs)
{
	char filename[MIDDLE_BUFFER];
	char freq_str[SMALL_BUFFER];
	FILE *f;
	int cpu_n = 0;

	//clean data array
	for (cpu_n = 0; cpu_n < num_of_cpu; cpu_n++)
		freqs[cpu_n] = 0.0;

	cpu_n = 0;
	while (1) {
		//is CPU present
		snprintf(filename, MIDDLE_BUFFER,
			 "/sys/devices/system/cpu/cpu%d/online", cpu_n);

		f = fopen(filename, "r");
		if (!f){
			LOGI_th_samp("file not found <%s\n>", filename);
			break;
		}
		fclose(f);

		//get CPU freq
		snprintf(filename, MIDDLE_BUFFER,
			 "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_cur_freq", cpu_n);
		f = fopen(filename, "r");
		if (!f)
		{
			//core is disabled
			LOGI_th_samp("core #%d diasabled\n", cpu_n);
			freqs[cpu_n] = 0.0;
		} else {
			//core enabled, get frequency
			fscanf(f, "%s", freq_str);
			freqs[cpu_n] = atof(freq_str);
			LOGI_th_samp("core #%d freq = %.0f\n", cpu_n, freqs[cpu_n]);
			fclose(f);
		}

		//next core
		cpu_n++;

	}
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
/* 	LOGI("dell t=%d\n",t); */
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

/* 	LOGI("ret 0\n"); */
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

	if(unlikely(fd == -1)){
		return -1;
	}

	num = read(fd, buf, BUFFER_MAX);
	close(fd);

	if(unlikely(num <= 0)){
		LOGE("nothing read from '%s'\n", filename);
		return -1;
	} else if(num == BUFFER_MAX)
		num -= 1;


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
	P->sh_mem = 0;

	// read from smaps file
	sprintf(filename, "%s/smaps", path);
	fp = fopen(filename, "r");

	if(fp == NULL){
		return -1;
	}

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
			else if(strncmp(buf, "Shared", 6) == 0)	// line is started with "Shared"
			{
				char *p = strstr(buf, ":");
				if (p != 0) {
					sscanf(p, ":%s kB", numbuf);
					P->sh_mem += atoi(numbuf);
				}

			}
			else	// not Pss line
			{
				if (is_probe_so == 0 && strlen(buf) > MIN_SMAP_BLOCKLINE)
				{
					// first we find probe so section
					if(strstr(buf, DA_PROBE_TIZEN_SONAME) != NULL ||
							strstr(buf, DA_PROBE_OSP_SONAME) != NULL)
					{
						// found probe.so
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
			else if(strncmp(buf, "Shared", 6) == 0)	// line is started with "Shared"
			{
				char *p = strstr(buf, ":");
				if (p != 0) {
					sscanf(p, ":%s kB", numbuf);
					P->sh_mem += atoi(numbuf);
				}
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
/* 		LOGI("#%d\n", i); */
		if (pidarray[i] == 0)	// pid is invalid
		{
			ret = 1;
			continue;
		}

		sprintf(buf, "/proc/%d", pidarray[i]);
/* 		LOGI("#->%s\n", buf); */
		if (unlikely(stat(buf, &sb) == -1))	// cannot access anymore
		{
/* 			LOGI("#del from prochead=%d\n", prochead); */
			del_node(&prochead, pidarray[i]);
			ret = errno;
			continue;
		}

/* 		LOGI("find node = %d\n", procnode); */
		if ((procnode = find_node(prochead, pidarray[i])) == NULL)	// new process
		{
/* 			LOGI("proc node1 = %d\n", procnode); */
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
/* 			LOGI("proc node2 = %d\n", procnode); */
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
/* 	LOGI("del_notfound_node\n"); */
	del_notfound_node(&prochead);
/* 	LOGI("reset_found_node\n"); */
	reset_found_node(prochead);

/* 	LOGI("ret %d\n", ret); */
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
			/* This file may absent in the system */
		}

		for(i = 0; i < num_of_cpu; i++)
		{
			if(cpus[i].pfreq == NULL && num_of_freq)
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
static void init_system_cpu_data()
{
	manager.fd.procstat = fopen(PROCSTAT, "r");
}

static int update_system_cpu_data(int cur_index)
{
/* 	LOGI(">\n"); */

	FILE* fp = manager.fd.procstat;
	int num;
	char buf[BUFFER_MAX];

	if(fp == NULL)
		return -1;

	rewind(fp);
	fflush(fp);

	if(fgets(buf, sizeof(buf), fp) == NULL)
	{
		LOGE("Failed to read first line of " PROCSTAT "\n");
		return -1;
	}

/* LOGI("scan; cpus = %d\n", cpus); */

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
		return -1;
	}

#ifdef FOR_EACH_CPU

/* 	LOGI("cpu num = %d\n", num_of_cpu); */
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
					LOGI_th_samp("Readed %d stats of %dth cpu\n", num, i);
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
		return -1;
	}
#endif
	return 0;
}

// return 0 for normal case
// return negative value for error
static void init_update_system_memory_data()
{
	manager.fd.procmeminfo = open(PROCMEMINFO, O_RDONLY);
}

static int update_system_memory_data(uint64_t *memtotal, uint64_t *memused)
{
	int meminfo_fd = manager.fd.procmeminfo;
	char *head, *tail;
	int i, num;
	char buf[BUFFER_MAX];
	static const mem_t mem_table[] = {
		{"Buffers",		(unsigned long *)&mem_slot_array[MEM_SLOT_BUFFER]},
		{"Cached",		(unsigned long *)&mem_slot_array[MEM_SLOT_CACHED]},
		{"MemFree",		(unsigned long *)&mem_slot_array[MEM_SLOT_FREE]},
		{"MemTotal",	(unsigned long *)&mem_slot_array[MEM_SLOT_TOTAL]},
	};
	const int mem_table_size = sizeof(mem_table) / sizeof(mem_t);

	if (meminfo_fd == -1)
		return -1;

	lseek(meminfo_fd, 0L, SEEK_SET);
	if((num = read(meminfo_fd, buf, BUFFER_MAX)) < 0)
	{
		LOGE("Failed to read from " PROCMEMINFO "\n");
		return -1;
	}

	if(num == BUFFER_MAX)
		num -= 1;

	buf[num] = '\0';
//	LOGI("buffer=<%s>\n", buf);

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
		{
			head = tail + 1;
		}
		tail = strchr(head, '\n');
		if(tail == NULL)
			break;
		head = tail + 1;
	}
/* 		LOGI("Buffers = %016LX\n", mem_slot_array[MEM_SLOT_BUFFER]); */
/* 		LOGI("Cached  = %016LX\n", mem_slot_array[MEM_SLOT_CACHED]); */
/* 		LOGI("MemFree = %016LX\n", mem_slot_array[MEM_SLOT_FREE]); */
/* 		LOGI("MemTotal= %016LX\n", mem_slot_array[MEM_SLOT_TOTAL]); */
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
//static
static unsigned long get_system_total_memory(void)
{
	int meminfo_fd = manager.fd.procmeminfo;
	char *head, *tail;
	int num;
	char buf[BUFFER_MAX];
	static const char* memtotalstr = "MemTotal";
	unsigned long totalmem = 0;

	if (meminfo_fd == -1)
		return 0;

	lseek(meminfo_fd, 0L, SEEK_SET);

	if((num = read(meminfo_fd, buf, BUFFER_MAX)) < 0)
	{
		LOGE("Failed to read from " PROCMEMINFO "\n");
		return 0;
	}

	if(num == BUFFER_MAX)
		num -= 1;
	buf[num] = '\0';

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

/* 	LOGI("File storage total(%d), free(%d)\n", total, free); */
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
		LOGI_th_samp("total_used_drive = -1\n");
		return -1;
	}

	free = storage + card;
	total = get_total_drive() - free;

	LOGI_th_samp("total_used_drive = %d\n", total);

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
		LOGE("task not found '%s'\n", path);
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
				if (unlikely((ret = parse_proc_stat_file_bypid(buf, &(procnode->proc_data))) < 0))
				{
					LOGE("Failed to get proc stat file by tid(%d). add node\n", tid);
				}
				else
				{
					procnode->saved_utime = procnode->proc_data.utime;
					procnode->saved_stime = procnode->proc_data.stime;
/* 					LOGI("data updated\n"); */
				}
			}
			else
			{
				if (unlikely((ret = parse_proc_stat_file_bypid(buf, &(procnode->proc_data))) < 0))
				{
					LOGE("Failed to get proc stat file by tid(%d). node exist\n", tid);
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

static int get_device_network_type(char* buf, int buflen)
{
	int len = 0;

	if (is_cdma_available())
		len += sprintf(buf + len, "CDMA,");

	if (is_edge_available())
		len += sprintf(buf + len, "EDGE,");

	if (is_gprs_available())
		len += sprintf(buf + len, "GPRS,");

	if (is_gsm_available())
		len += sprintf(buf + len, "GSM,");

	if (is_hsdpa_available())
		len += sprintf(buf + len, "HSDPA,");

	if (is_hspa_available())
		len += sprintf(buf + len, "HSPA,");

	if (is_hsupa_available())
		len += sprintf(buf + len, "HSUPA,");

	if (is_umts_available())
		len += sprintf(buf + len, "UMTS,");

	if (is_lte_available())
		len += sprintf(buf + len, "LTE,");

	if (len != 0)
		buf[--len] = 0;

	return len;
}

static int update_cpus_info(int event_num, float elapsed)
{
	int i = 0;
	CPU_t* cpuptr;
	// calculate for system cpu load
#ifdef FOR_EACH_CPU
	for(i = 0; i < num_of_cpu; i++)
#else
	for(i = num_of_cpu; i <= num_of_cpu; i++)
#endif
	{
	LOGI_th_samp("CPU #%d\n", i);
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
					cpuptr->cpu_usage = (1.0f - ((float)cpuptr->idle_ticks /
								(float)cpuptr->total_ticks)) * 100.0f;
				}
//				if(i != num_of_cpu)
//				{
//					idle_tick_sum += cpuptr->idle_ticks;
//					total_tick_sum += cpuptr->total_ticks;
//				}
				LOGI_th_samp("System cpu usage log : %d, %Ld, %Ld\n",
						i, cpuptr->idle_ticks, cpuptr->total_ticks);
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
				cpuptr->cpu_usage = ((float)cpuptr->idle_ticks /
						(float)cpuptr->total_ticks) * 100.0f;
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
	return 0;
}

static int fill_system_processes_info(float factor, struct system_info_t * sys_info)
{
	procNode* proc;
	int i = 0;
	float thread_load;
	// data variable
	unsigned long virtual = 0;
	unsigned long resident = 0;
	unsigned long shared = 0;
	unsigned long pssmem = 0;
	uint64_t ticks = 0;
	float app_cpu_usage = 0.0;

	LOGI_th_samp("prochead = %X\n", (unsigned int)prochead);
	for(proc = prochead; proc != NULL; proc = proc->next)
	{
		//increment process count
		sys_info->count_of_processes++; //maybe wrong
	}
	sys_info->process_load = malloc (
										sys_info->count_of_processes *
										sizeof(*sys_info->process_load)
									);
	i = 0;
	for(proc = prochead; proc != NULL; proc = proc->next)
	{
		LOGI_th_samp("proc#%d (%d %d),(%d %d) (%d) %f\n",
				i,
				(unsigned int)proc->proc_data.utime, (unsigned int)proc->proc_data.stime ,
				(unsigned int)proc->saved_utime, (unsigned int)proc->saved_stime,
				(int)(proc->proc_data.utime + proc->proc_data.stime - proc->saved_utime - proc->saved_stime),
				(float)(
					proc->saved_utime + proc->saved_stime -
					proc->proc_data.utime - proc->proc_data.stime
					) * factor);
		thread_load = (float)(proc->proc_data.utime + proc->proc_data.stime -
								proc->saved_utime - proc->saved_stime) * factor;

		if(thread_load > 100.0f)
			thread_load = 100.0f;

		//num_thread += proc->proc_data.numofthread;
		virtual += proc->proc_data.vir_mem;					// Byte
		resident += (proc->proc_data.res_memblock * 4);		// KByte
		pssmem += (proc->proc_data.pss);					// KByte
		shared += (proc->proc_data.sh_mem);					// KByte
		ticks += (proc->proc_data.utime + proc->proc_data.stime -
					proc->saved_utime - proc->saved_stime);

		proc->saved_utime = proc->proc_data.utime;
		proc->saved_stime = proc->proc_data.stime;


		sys_info->process_load[i].id = proc->proc_data.pid;
		sys_info->process_load[i].load = thread_load;
		i++;

	}

	app_cpu_usage = (float)ticks * factor;
	if(app_cpu_usage > 100.0f)
		app_cpu_usage = 100.0f;
	resident = resident * 1024;		// change to Byte
	pssmem = pssmem * 1024;			// change to Byte
	shared = shared * 1024;			// change to Byte

	sys_info->virtual_memory = virtual;
	sys_info->resident_memory = resident;
	sys_info->shared_memory = shared;
	sys_info->pss_memory = pssmem;

	sys_info->app_cpu_usage = app_cpu_usage;

	return 0;

}

// fill threads information
static int fill_system_threads_info(float factor, struct system_info_t * sys_info)
{
	procNode* proc;
	float thread_load;

	for(proc = thread_prochead; proc != NULL; proc = proc->next)
		//increment thread count
		sys_info->count_of_threads++; //maybe wrong

/* 	LOGI_th_samp("thread load\n"); */
	struct thread_info_t *pthread;
	if (sys_info->count_of_threads != 0)
	{
		sys_info->thread_load = malloc( sys_info->count_of_threads * sizeof(*sys_info->thread_load) );
		pthread = sys_info->thread_load;
	}

	for(proc = thread_prochead; proc != NULL; proc = proc->next)
	{
		thread_load = (float)(proc->proc_data.utime + proc->proc_data.stime -
								proc->saved_utime - proc->saved_stime)
			* factor;
		if(thread_load > 100.0f)
			thread_load = 100.0f;

		pthread->pid = proc->proc_data.pid;
		pthread->load = thread_load;
		pthread++;

//		sprintf(thread_loadtmpbuf, "%d,%.1f,", proc->proc_data.pid, thread_load);
//		strcat(thread_loadbuf, thread_loadtmpbuf);

		proc->saved_utime = proc->proc_data.utime;
		proc->saved_stime = proc->proc_data.stime;
	}

	return 0;
}

//fill system cpu information
static int fill_system_cpu_info(struct system_info_t *sys_info)
{
	float sys_usage = 0.0f;
	int i = 0;

	// calculate for whole cpu load by average all core load
	LOGI_th_samp("calculate for whole cpu load num_of_cpu=%d\n", num_of_cpu);
	for(i = 0 ; i < num_of_cpu; i++)
		sys_usage += cpus[i].cpu_usage;

	// fill cpu load
	float *pcpu_usage;
	if (num_of_cpu != 0)
	{
		sys_info->cpu_load = malloc( num_of_cpu * sizeof(*sys_info->cpu_load) );
		pcpu_usage = sys_info->cpu_load;
		for(i = 0; i < num_of_cpu; i++)
		{
			LOGI_th_samp("cpu#%d : %.1f\n" , i,  cpus[i].cpu_usage);
			*pcpu_usage = cpus[i].cpu_usage;
			pcpu_usage++;
		}
	}

	//fill CPU frequency
	sys_info->cpu_frequency = malloc(num_of_cpu * sizeof(float));
	if (!sys_info->cpu_frequency) {
		LOGE("Cannot alloc cpu freq\n");
		return 1;
	}
	get_cpu_frequency(sys_info->cpu_frequency);

	return 0;
}

static void skip_lines(FILE * fp, unsigned int count)
{
	char *buffer = NULL;
	size_t buflen;
	unsigned int index;
	for (index = 0; index != count; ++index)
		getline(&buffer, &buflen, fp);
	free(buffer);
}

static void skip_tokens(FILE * fp, unsigned int count)
{
	unsigned int index;

	for (index = 0; index != count; ++index)
		fscanf(fp, "%*s");
}

static void init_network_stat()
{
	manager.fd.networkstat = fopen("/proc/net/dev", "r");
}

static void get_network_stat(uint32_t *recv, uint32_t *send)
{
	FILE *fp = manager.fd.networkstat;
	uintmax_t irecv, isend;
	char ifname[64];
	if (fp == NULL)
		return;

	rewind(fp);
	fflush(fp);

	*recv = *send = 0;
	skip_lines(fp, 2);	/* strip header */

	while (fscanf(fp, "%s", ifname) != EOF)
		if (strcmp("lo:", ifname)) {
			fscanf(fp, "%" SCNuMAX, &irecv);
			skip_tokens(fp, 7);

			fscanf(fp, "%" SCNuMAX, &isend);
			skip_tokens(fp, 7);

			*recv += irecv;
			*send += isend;
		} else
			skip_tokens(fp, 16);
}

static void peek_network_stat_diff(uint32_t *recv, uint32_t *send)
{
	static uint32_t irecv_old, isend_old;
	uint32_t tmp;

	get_network_stat(recv, send);

	tmp = *recv;
	*recv = val_diff(tmp, irecv_old);
	irecv_old = tmp;

	tmp = *send;
	*send = val_diff(tmp, isend_old);
	isend_old = tmp;

}

static void init_disk_stat(void)
{
	manager.fd.diskstats = fopen("/proc/diskstats", "r");
}

static void get_disk_stat(uint32_t *reads, uint32_t *sec_reads,
			  uint32_t *writes, uint32_t *sec_writes)
{
	enum { partition_name_maxlength = 128 };
	FILE *fp = manager.fd.diskstats;
	char master_partition[partition_name_maxlength] = { 0 };

	*reads = *writes = 0;
	*sec_reads = *sec_writes = 0;

	if (fp == NULL)
		return;


	rewind(fp);
	fflush(fp);

	while (!feof(fp)) {
		char partition[partition_name_maxlength];
		uintmax_t preads, pwrites;
		uintmax_t psec_read, psec_write;
		skip_tokens(fp, 2);
		fscanf(fp, "%s", partition);
		if (*master_partition
		    && !strncmp(master_partition, partition,
			       strlen(master_partition))) {
			/* subpartition */
			skip_tokens(fp, 11);
		} else {
			//1
			fscanf(fp, "%" SCNuMAX, &preads);
			skip_tokens(fp, 1);
			//3
			fscanf(fp, "%" SCNuMAX, &psec_read);
			skip_tokens(fp, 1);
			//5
			fscanf(fp, "%" SCNuMAX, &pwrites);
			skip_tokens(fp, 1);
			//7
			fscanf(fp, "%" SCNuMAX, &psec_write);
			skip_tokens(fp, 4);

			memcpy(master_partition, partition,
			       partition_name_maxlength);
			// FIXME rw size is in sectors
			// actualy different disks - different partition sector size (?)
			// maybe need convert to bytes like this:
			//*read += pread * sec_size;
			//*write += pwrite * sec_size;

			*reads += (uint32_t)preads;
			*writes += (uint32_t)pwrites;

			*sec_reads += (uint32_t)psec_read;
			*sec_writes += (uint32_t)psec_write;
		}
	}

}

static void peek_disk_stat_diff(uint32_t *reads, uint32_t *sec_reads,
			        uint32_t *writes, uint32_t *sec_writes)
{
	static uint32_t reads_old;
	static uint32_t sec_reads_old;
	static uint32_t writes_old;
	static uint32_t sec_writes_old;

	uint32_t tmp;

	//get cur values
	get_disk_stat(reads, sec_reads, writes, sec_writes);

	tmp = *reads;
	*reads = val_diff(tmp, reads_old);
	reads_old = tmp;

	tmp = *writes;
	*writes = val_diff(tmp, writes_old);
	writes_old = tmp;

	tmp = *sec_reads;
	*sec_reads = val_diff(tmp, sec_reads_old);
	sec_reads_old = tmp;

	tmp = *sec_writes;
	*sec_writes = val_diff(tmp, sec_writes_old);
	sec_writes_old = tmp;

}

static float get_elapsed(void)
{
	static struct timeval old_time = {0, 0};
	struct timeval current_time;
	float elapsed;

	gettimeofday(&current_time, NULL);
	elapsed = (current_time.tv_sec - old_time.tv_sec) +
		((float)(current_time.tv_usec - old_time.tv_usec) / 1000000.0f);
	old_time.tv_sec = current_time.tv_sec;
	old_time.tv_usec = current_time.tv_usec;

	return elapsed;
}

static float get_factor(float elapsed)
{
	return 100.0f / ((float)Hertz * elapsed * num_of_cpu);
}

static uint64_t read_int64_from_file(const char *fname)
{
	FILE *fp = fopen(fname, "r");
	uint64_t value;
	if (!fp)
		return 0;
	if (fscanf(fp, "%lld", &value) != 1)
		value = 0;
	fclose(fp);
	return value;
}

#define swap_sysfs_relpath(x) ("/sys/kernel/debug/swap/energy/" #x)
#define swap_read_int64(x) (read_int64_from_file(swap_sysfs_relpath(x)))
static uint64_t get_system_lcd_energy()
{
	static const char *PROC_LCD_ENERGY_FILES_GLOBPATTERN =
		"/sys/kernel/debug/swap/energy/lcd/*/system";
	uint64_t sum_energy = 0;
	glob_t glob_buf;
	size_t i;
	const int err = glob(PROC_LCD_ENERGY_FILES_GLOBPATTERN, 0,
			     NULL, &glob_buf);

	if (err) {
		LOG_ONCE_E("Globbing for LCD failed with error %d\n", err);
		return 0;
	}

	for (i = 0; i < glob_buf.gl_pathc; ++i)
		sum_energy += read_int64_from_file(glob_buf.gl_pathv[i]);

	globfree(&glob_buf);
	return sum_energy;
}
/*
 * Calculates difference between current and previous sample (system).
 * Stores mutable state in static variables.
 */
static uint32_t pop_sys_energy_per_device(enum supported_device dev)
{
	static uint64_t cpu_old, flash_old, lcd_old;
	uint64_t cpu_new, flash_new, lcd_new;
	uint64_t cpu_diff, flash_diff, lcd_diff;

	switch (dev) {
	case DEVICE_CPU:
		cpu_new = swap_read_int64(cpu_idle/system) +
			swap_read_int64(cpu_running/system);
		cpu_diff = val_diff(cpu_new, cpu_old);
		cpu_old = cpu_new;
		return (uint32_t)cpu_diff;

	case DEVICE_FLASH:
		flash_new = swap_read_int64(flash_read/system) +
			swap_read_int64(flash_write/system);
		flash_diff = val_diff(flash_new, flash_old);
		flash_old = flash_new;
		return (uint32_t)flash_diff;
	case DEVICE_LCD:
		lcd_new = get_system_lcd_energy();
		lcd_diff = val_diff(lcd_new, lcd_old);
		lcd_old = lcd_new;
		return (uint32_t)lcd_diff;
	default:
		assert(0 && "Unknown device. This should not happen");
		return -41;
	}
}

// Calculates difference between current and previous sample (app).
// Stores mutable state in static variables.
static uint32_t pop_app_energy_per_device(enum supported_device dev)
{
	static uint64_t cpu_old, flash_old;
	uint64_t cpu_new, flash_new;
	uint64_t cpu_diff, flash_diff;

	switch (dev) {
	case DEVICE_CPU:
		cpu_new = swap_read_int64(cpu_running/apps);
		cpu_diff = val_diff(cpu_new, cpu_old);
		cpu_old = cpu_new;
		return (uint32_t)cpu_diff;
	case DEVICE_FLASH:
		flash_new = swap_read_int64(flash_read/apps) +
			swap_read_int64(flash_write/apps);
		flash_diff = val_diff(flash_new, flash_old);
		flash_old = flash_new;
		return (uint32_t)flash_diff;
	case DEVICE_LCD:
		/**
		 * Per-application energy accounting
		 * is not supported for LCD.
		 */
		return 0;
	default:
		assert(0 && "Unknown device. This should not happen");
		return -41;
	}
}

static int get_pid_array(int arr[], const int n)
{
	DIR *d = opendir("/proc");
	struct dirent *dirent;
	int count = 0;

	if (!d) {
		LOGW("Cannot open /proc dir (%s)\n", strerror(errno));
		return 0;
	}

	while ((dirent = readdir(d)) && (count < n)) {
		if (dirent->d_type == DT_DIR) {
			char *tmp;
			pid_t pid = strtol(dirent->d_name, &tmp, 10);
			if (*tmp == '\0')
				arr[count++] = pid;
		}
	}

	closedir(d);

	return count;
}

static pid_t get_first_target_process(void)
{
	pid_t pid = -1;
	int i;

	for (i = 0; i < MAX_TARGET_COUNT; i++) {
		if (manager.target[i].socket != -1 &&
		    manager.target[i].pid != -1) {
			pid = manager.target[i].pid;
			break;
		}
	}

	return pid;
}

// return log length (>0) for normal case
// return negative value for error
int get_system_info(struct system_info_t *sys_info)
{
	static int event_num = 0;
	uint64_t sysmemtotal = 0;
	uint64_t sysmemused = 0;
	int res = 0;
	float elapsed;
	float factor;

	LOGI_th_samp("start\n");

	memset(sys_info, 0, sizeof(*sys_info));

	// common (cpu, processes, memory)
	if (IS_OPT_SET(FL_CPU) ||
	    IS_OPT_SET(FL_PROCESSES) ||
	    IS_OPT_SET(FL_MEMORY)) {
		const int max_pid_num = 1024; /* ugly hardcode */
		int pidarray[max_pid_num];
		int pidcount = 0;
		pid_t first_target_pid = -1;

		pidcount = get_pid_array(pidarray, max_pid_num);
		LOGI_th_samp("PID count : %d\n", pidcount);

		if (update_process_data(pidarray, pidcount, PROCDATA_STAT) < 0) {
			LOGE("Failed to update process stat data\n");
			goto fail_exit;
		}
		/**
		 * This position is optimized position of timestamp. Just
		 * before get system cpu data and just after get process cpu
		 * data because cpu data is changed so fast and variance is so
		 * remarkable
		 */
		elapsed = get_elapsed(); /* DO NOT MOVE THIS SENTENCE! */
		factor = get_factor(elapsed);

		if (update_system_cpu_data(event_num) < 0) {
			LOGE("Failed to update system cpu data\n");
			goto fail_exit;
		}

		if (update_system_cpu_frequency(event_num) < 0) {
			LOGE("Failed to update system cpu freq data\n");
			goto fail_exit;
		}

		/**
		 * Memory data is changed slowly and variance is not
		 * remarkable, so memory data is less related with timestamp
		 * then cpu data
		 */
		if (update_process_data(pidarray, pidcount, PROCDATA_SMAPS) < 0) {
			LOGE("Failed to update process smaps data\n");
			goto fail_exit;
		}

		first_target_pid = get_first_target_process();
		if (first_target_pid > 0)
			if (update_thread_data(first_target_pid) < 0) {
				LOGE("Failed to update thread stat data\n");
				goto fail_exit;
			}

		if (update_system_memory_data(&sysmemtotal, &sysmemused) < 0) {
			LOGE("Failed to update system memory data\n");
			goto fail_exit;
		}

		if (update_cpus_info(event_num, elapsed) < 0) {
			LOGE("Failed to update cpus info\n");
			goto fail_exit;
		}

		/* calculate process load, memory, app_cpu_usage */
		if (fill_system_processes_info(factor, sys_info) < 0) {
			LOGE("Failed to fill processes info\n");
			goto fail_exit;
		}

		/* calculate thread load */
		if (fill_system_threads_info(factor, sys_info) < 0) {
			LOGE("Failed to fill threads info\n");
			goto fail_exit;
		}

		if (fill_system_cpu_info(sys_info) < 0) {
			LOGE("Failed to fill threads info\n");
			goto fail_exit;
		}
	}

	if (IS_OPT_SET(FL_MEMORY)) {
		sys_info->total_alloc_size = get_total_alloc_size();
		sys_info->system_memory_total = sysmemtotal;
		sys_info->system_memory_used = sysmemused;
	}

	LOGI_th_samp("Fill result structure\n");

	if (IS_OPT_SET(FL_DISK)) {
		sys_info->total_used_drive = get_total_used_drive();
		peek_disk_stat_diff(&sys_info->disk_reads,
				    &sys_info->disk_sectors_read,
				    &sys_info->disk_writes,
				    &sys_info->disk_sectors_write);
	}

	if (IS_OPT_SET(FL_NETWORK))
		peek_network_stat_diff(&sys_info->network_send_size,
				       &sys_info->network_receive_size);

	if (IS_OPT_SET(FL_DEVICE)) {
		sys_info->wifi_status = get_wifi_status();
		sys_info->bt_status = get_bt_status();
		sys_info->gps_status = get_gps_status();
		sys_info->brightness_status = get_brightness_status();
		sys_info->camera_status = get_camera_status();
		sys_info->sound_status = get_sound_status();
		sys_info->audio_status = get_audio_status();
		sys_info->vibration_status = get_vibration_status();
		sys_info->voltage_status = get_voltage_status();
		sys_info->rssi_status = get_rssi_status();
		sys_info->video_status = get_video_status();
		sys_info->call_status = get_call_status();
		sys_info->dnet_status = get_dnet_status();
	}

	if (IS_OPT_SET(FL_ENERGY)) {
		int i;
		sys_info->energy = 0; /* not implemented */
		for (i = 0; i != supported_devices_count; ++i) {
			sys_info->energy_per_device[i] =
				pop_sys_energy_per_device(i);
			sys_info->app_energy_per_device[i] = (i == DEVICE_LCD)
				? sys_info->energy_per_device[i]
				: pop_app_energy_per_device(i);
		}
	}

#ifdef THREAD_SAMPLING_DEBUG
	print_sys_info(sys_info);
#endif

	event_num++;
	LOGI_th_samp("exit\n");
	return res;

fail_exit:
	/* Some data corrupted. Free allocated data. */
	reset_system_info(sys_info);
	LOGI_th_samp("fail exit\n");
	return -1;
}

int initialize_system_info(void)
{
	int i;

	num_of_cpu = sysconf(_SC_NPROCESSORS_CONF);
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

int finalize_system_info(void)
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

static void test_and_close(int *fd)
{
	if (*fd > 0)
		close(*fd);
	*fd = -1;
}

static void ftest_and_close(FILE **fd)
{
	if (*fd != NULL)
		fclose(*fd);
	*fd = NULL;
}

#define strr(x) #x
#define str(x) strr(x)
#define dtest_and_close(fd) do {LOGI("CLOSE " str(fd) "\n");test_and_close(fd);} while(0)
#define dftest_and_close(fd) do {LOGI("CLOSE " str(fd) "\n");ftest_and_close(fd);} while(0)
void close_system_file_descriptors(void)
{
	dtest_and_close(&manager.fd.brightness);
	dtest_and_close(&manager.fd.voltage);
	dtest_and_close(&manager.fd.procmeminfo);

	dftest_and_close(&manager.fd.video);
	dftest_and_close(&manager.fd.procstat);
	dftest_and_close(&manager.fd.networkstat);
	dftest_and_close(&manager.fd.diskstats);
}

int init_system_file_descriptors(void)
{
	//inits
	init_brightness_status();
	init_voltage_status();
	init_update_system_memory_data();

	init_video_status();
	init_system_cpu_data();
	init_network_stat();
	init_disk_stat();

	if (manager.fd.brightness < 0)
		LOGW("brightness file not found\n");
	if (manager.fd.voltage < 0)
		LOGW("voltage file not found\n");
	if (manager.fd.procmeminfo < 0)
		LOGW("procmeminfo file not found\n");

	if (manager.fd.video == NULL)
		LOGW("video file not found\n");
	if (manager.fd.procstat == NULL)
		LOGW("procstat file not found\n");
	if (manager.fd.networkstat == NULL)
		LOGW("networkstat file not found\n");
	if (manager.fd.diskstats == NULL)
		LOGW("diskstat file not found\n");
	return 0;
}

//CMD SOCKET FUNCTIONS
int fill_target_info(struct target_info_t *target_info)
{
	/* system_info_get_value_bool() changes only 1 byte
	   so we need to be sure that the integer as a whole is correct */
	target_info->bluetooth_supp = 0;
	target_info->gps_supp = 0;
	target_info->wifi_supp = 0;
	target_info->camera_count = 0;
	target_info->network_type[0] = 0;

	target_info->sys_mem_size = get_system_total_memory();
	target_info->storage_size = stat_get_storageinfo(FSINFO_TYPE_TOTAL) *
		1024 * 1024;

	target_info->bluetooth_supp = is_bluetooth_available();
	target_info->gps_supp = is_gps_available();
	target_info->wifi_supp = is_wifi_available();

	target_info->camera_count = get_camera_count();

	get_device_network_type(target_info->network_type, NWTYPE_SIZE);


	target_info->max_brightness = get_max_brightness();
	target_info->cpu_core_count = sysconf(_SC_NPROCESSORS_CONF);
	return 0;
}

int sys_stat_prepare(void)
{
	uint32_t reads, writes, sec_reads, sec_writes;
	uint32_t recv, send;

	peek_disk_stat_diff(&reads, &writes, &sec_reads, &sec_writes);
	peek_network_stat_diff(&recv, &send);

	return 0;
}

static uint32_t msg_data_payload_length(const struct system_info_t *sys_info)
{
	uint32_t len = sizeof(*sys_info);

	/* num_of_cpu is unknown at compile time */
	len += 2 * num_of_cpu * sizeof(float);

	/* subtract pointers */
	len -= sizeof(sys_info->cpu_frequency) + sizeof(sys_info->cpu_load);
	len -= sizeof(sys_info->thread_load) + sizeof(sys_info->process_load);

	if (IS_OPT_SET(FL_CPU))
		len += sys_info->count_of_threads *
			sizeof(*sys_info->thread_load);

	if (IS_OPT_SET(FL_PROCESSES))
		len += sys_info->count_of_processes *
			sizeof(*sys_info->process_load);

	return len;
}

struct msg_data_t *pack_system_info(struct system_info_t *sys_info)
{
	const int len = msg_data_payload_length(sys_info);
	struct msg_data_t *msg = NULL;
	char *p = NULL;
	int i = 0;

	msg = malloc(MSG_DATA_HDR_LEN + len);
	if (!msg) {
		LOGE("Cannot alloc message: %d bytes\n", len);
		return NULL;
	}

	fill_data_msg_head(msg, NMSG_SYSTEM, 0, len);
	p = msg->payload;

	// CPU
	if (IS_OPT_SET(FL_CPU)) {
		pack_float(p, sys_info->app_cpu_usage);

		for (i = 0; i < num_of_cpu; i++) {
			if (sys_info->cpu_frequency)
				pack_float(p, sys_info->cpu_frequency[i]);
			else
				pack_float(p, 0.0);
		}

		for (i = 0; i < num_of_cpu; i++) {
			if (sys_info->cpu_load)
				pack_float(p, sys_info->cpu_load[i]);
			else
				pack_float(p, 0.0);
		}
		// thread
		pack_int32(p, sys_info->count_of_threads);
		for (i = 0; i < sys_info->count_of_threads; i++) {
			if (sys_info->thread_load) {
				pack_int32(p, sys_info->thread_load[i].pid);
				pack_float(p, sys_info->thread_load[i].load);
			} else {
				pack_int32(p, 0);
				pack_float(p, 0.0);
			}
		}
	} else {
		pack_float(p, 0.0); /* pack app_cpu_usage */

		for (i = 0; i < num_of_cpu; i++) {
			pack_float(p, 0.0); /* pack cpu_frequency */
			pack_float(p, 0.0); /* pack cpu_load */
		}
		/* thread */
		pack_int32(p, 0); /* pack count_of_threads */
	}

	/* process */
	if (IS_OPT_SET(FL_PROCESSES)) {
		pack_int32(p, sys_info->count_of_processes);
		for (i = 0; i < sys_info->count_of_processes; i++) {
			if (sys_info->process_load) {
				pack_int32(p, sys_info->process_load[i].id);
				pack_float(p, sys_info->process_load[i].load);
			} else {
				pack_int32(p, 0);
				pack_float(p, 0.0);
			}
		}
	} else {
		pack_int32(p, 0); /* pack count_of_processes */
	}

	/* memory */
	if (IS_OPT_SET(FL_MEMORY)) {
		pack_int32(p, sys_info->virtual_memory);
		pack_int32(p, sys_info->resident_memory);
		pack_int32(p, sys_info->shared_memory);
		pack_int32(p, sys_info->pss_memory);
		pack_int64(p, sys_info->total_alloc_size);
		pack_int64(p, sys_info->system_memory_total);
		pack_int64(p, sys_info->system_memory_used);
	} else {
		pack_int32(p, 0); /* pack virtual_memory */
		pack_int32(p, 0); /* pack resident_memory */
		pack_int32(p, 0); /* pack shared_memory */
		pack_int32(p, 0); /* pack pss_memory */
		pack_int64(p, (uint64_t) 0); /* pack total_alloc_size */
		pack_int64(p, (uint64_t) 0); /* pack system_memory_total */
		pack_int64(p, (uint64_t) 0); /* pack system_memory_used */
	}

	pack_int32(p, sys_info->total_used_drive);
	pack_int32(p, sys_info->disk_reads);
	pack_int32(p, sys_info->disk_sectors_read);
	pack_int32(p, sys_info->disk_writes);
	pack_int32(p, sys_info->disk_sectors_write);

	pack_int32(p, sys_info->network_send_size);
	pack_int32(p, sys_info->network_receive_size);

	pack_int32(p, sys_info->wifi_status);
	pack_int32(p, sys_info->bt_status);
	pack_int32(p, sys_info->gps_status);
	pack_int32(p, sys_info->brightness_status);
	pack_int32(p, sys_info->camera_status);
	pack_int32(p, sys_info->sound_status);
	pack_int32(p, sys_info->audio_status);
	pack_int32(p, sys_info->vibration_status);
	pack_int32(p, sys_info->voltage_status);
	pack_int32(p, sys_info->rssi_status);
	pack_int32(p, sys_info->video_status);
	pack_int32(p, sys_info->call_status);
	pack_int32(p, sys_info->dnet_status);

	pack_int32(p, sys_info->energy);
	for (i = 0; i != supported_devices_count; ++i)
		pack_int32(p, sys_info->energy_per_device[i]);
	for (i = 0; i != supported_devices_count; ++i)
		pack_int32(p, sys_info->app_energy_per_device[i]);

	return msg;
}
