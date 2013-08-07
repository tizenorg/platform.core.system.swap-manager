/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
 * Nikita Kalyazin    <:wn.kalyazin@samsung.com>
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


#include "da_protocol.h"
#include "daemon.h"
#include "da_data.h"
#include "debug.h"

#include <sys/time.h>

int print_sys_info(struct system_info_t * sys_info)
{
	int i = 0;
	/* //FOR DEBUG
	sys_info->energy=0x1;
	sys_info->wifi_status=0x2;
	sys_info->bt_status=0x3;
	sys_info->gps_status=0x4;
	sys_info->brightness_status=0x5;

	sys_info->camera_status=0x6;
	sys_info->sound_status=0x7;
	sys_info->audio_status=0x8;
	sys_info->vibration_status=0x9;
	sys_info->voltage_status=0x10;

	sys_info->rssi_status=0x11;
	sys_info->video_status=0x12;
	sys_info->call_status=0x13;
	sys_info->dnet_status=0x14;
	sys_info->cpu_frequency=0x15;

	sys_info->app_cpu_usage=0.0;
	sys_info->cpu_load=0x17;
	sys_info->virtual_memory=0x18;
	sys_info->resident_memory=0x19;
	sys_info->shared_memory=0x20;

	sys_info->pss_memory=0x21;
	sys_info->total_alloc_size=(uint32_t)0x22;
	sys_info->system_memory_total=(uint64_t)0x23;
	sys_info->system_memory_used=(uint64_t)0x24;
	sys_info->total_used_drive=0x25;

	sys_info->count_of_threads=0x26;
	sys_info->thread_load=0x27;
	sys_info->count_of_processes=0x28;
	sys_info->process_load=0x29;
	sys_info->disk_read_size=0x30;

	sys_info->disk_write_size=0x31;
	sys_info->network_send_size=0x32;
	sys_info->network_receive_size=0x33;
	*/

	LOGI("isysinfo:\n\
\
	energy = 0x%X\n\
	wifi_status = 0x%X\n\
	bt_status = 0x%X\n\
	gps_status = 0x%X\n\
	brightness_status = 0x%X\n\
\
	camera_status = 0x%X\n\
	sound_status = 0x%X\n\
	audio_status = 0x%X\n\
	vibration_status = 0x%X\n\
	voltage_status = 0x%X\n\
\
	rssi_status = 0x%X\n\
	video_status = 0x%X\n\
	call_status = 0x%X\n\
	dnet_status = 0x%X\n\
	cpu_frequency (pointer)= 0x%X\n\
\
	app_cpu_usage = %f\n\
	cpu_load (pointer)= 0x%X\n\
	virtual_memory = 0x%X\n\
	resident_memory = 0x%X\n\
	shared_memory = 0x%X\n\
\
	pss_memory = 0x%X\n\
	total_alloc_size = 0x%X\n\
	system_memory_total = %llu\n\
	system_memory_used = %llu\n\
	total_used_drive = %d\n\
\
	count_of_threads = %u\n\
	thread_load (pointer)= 0x%X\n\
	count_of_processes = %u\n\
	process_load = 0x%X\n\
	disk_reads = 0x%X\n\
\
	disk_sectors_read = 0x%X\n\
	disk_writes = 0x%X\n\
	disk_sectors_write = 0x%X\n\
	network_send_size = 0x%X\n\
	network_receive_size = 0x%X\n",

	sys_info->energy,
	sys_info->wifi_status,
	sys_info->bt_status,
	sys_info->gps_status,
	sys_info->brightness_status,

	sys_info->camera_status,
	sys_info->sound_status,
	sys_info->audio_status,
	sys_info->vibration_status,
	sys_info->voltage_status,

	sys_info->rssi_status,
	sys_info->video_status,
	sys_info->call_status,
	sys_info->dnet_status,
	(unsigned int)sys_info->cpu_frequency,

	sys_info->app_cpu_usage,
	(unsigned int)sys_info->cpu_load,
	sys_info->virtual_memory,
	sys_info->resident_memory,
	sys_info->shared_memory,

	sys_info->pss_memory,
	sys_info->total_alloc_size,
	sys_info->system_memory_total,
	sys_info->system_memory_used,
	sys_info->total_used_drive,

	sys_info->count_of_threads,
	(unsigned int)sys_info->thread_load,
	sys_info->count_of_processes,
	(unsigned int)sys_info->process_load,
	sys_info->disk_reads,

	sys_info->disk_sectors_read,
	sys_info->disk_writes,
	sys_info->disk_sectors_write,
	sys_info->network_send_size,
	sys_info->network_receive_size
	);
	LOGI_("->\n");
	for ( i=0; i<sys_info->count_of_processes; i++)
	{
		LOGI_("\tpr %016X : %f\n",
		sys_info->process_load[i].id,
		sys_info->process_load[i].load
		);
	}
	for ( i=0; i<sys_info->count_of_threads; i++)
	{
		LOGI_("\tth %016X : %f\n",
		sys_info->thread_load[i].pid,
		sys_info->thread_load[i].load
		);
	}

	// FIXME CPU core num hardcoded
	for ( i=0; i<4; i++)
	{
		LOGI_("\tCPU load #%d : %f\n",
		i,
		sys_info->cpu_load[i]
		);
	}
	// FIXME CPU core num hardcoded
	for ( i=0; i<4; i++)
	{
		LOGI_("\tCPU freq #%d : %f\n",
		i,
		sys_info->cpu_frequency[i]
		);
	}
//	char *p = sys_info;
	printBuf((char*)sys_info, sizeof(*sys_info));

	return 0;
}

int fill_data_msg_head (struct msg_data_t *data, uint32_t msgid,
							uint32_t seq, uint32_t len)
{
	struct timeval time;
	data->id = msgid;
	data->seq_num = seq; // TODO fill good value
	gettimeofday(&time, NULL);
	data->sec = time.tv_sec;
	data->nsec = time.tv_usec * 1000;
	data->len = len;
	return 0;
}


//allocate memory, need free!!!
struct msg_data_t *gen_message_terminate(uint32_t id)
{
	uint32_t payload_len = sizeof(uint32_t);
	struct msg_data_t *data = malloc(sizeof(*data) + payload_len);
	char *p;
	fill_data_msg_head(data,NMSG_TERMINATE, 0, payload_len);
	// TODO fill good value
	p = data->payload;
	pack_int(p,id);
	return data;
}

//allocate memory, need free!!!
struct msg_data_t *gen_message_error(const char * err_msg)
{
	int payload_len = strlen(err_msg)+1;
	struct msg_data_t *data = malloc(sizeof(*data) + payload_len);
	char *p;
	fill_data_msg_head(data, NMSG_ERROR, 0, payload_len);

	p = data->payload;
	pack_str(p,err_msg);
	return data;
}

//allocatr memory, need free!!!
struct msg_data_t *gen_message_event(
		      struct input_event *events,
		      uint32_t events_count,
		      uint32_t id)
{
	uint32_t i = 0;
	uint32_t payload_len = events_count * (sizeof(id) * 4) +
							sizeof(events_count);
	struct msg_data_t *data = malloc(sizeof(*data) + payload_len);
	memset(data,0,sizeof(*data) + payload_len);
	char *p;

	fill_data_msg_head(data, NMSG_RECORD, 0, payload_len);

	p = data->payload;
	pack_int(p,events_count);

	for (i=0; i<events_count; i++){
		pack_int32(p,id);
		pack_int32(p,events[i].type);
		pack_int32(p,events[i].code);
		pack_int32(p,events[i].value);
	}

	return data;
}

void inline free_msg_data(struct msg_data_t *msg)
{
	free(msg);
};

