#include "da_protocol.h"
#include "da_data.h"
/*
struct msg_system_t msg_system;

int fill_message_system(struct msg_system_t ** sys){

	struct msg_system_t * psys = *sys;
	psys->energy = 0;
	psys->WiFi_status = get_wifi_status();
	psys->BT_status = get_bt_status();
	psys->GPS_status = get_gps_status();
	psys->brightness_status = 	get_brightness_status();
	psys->camera_status = get_camera_status();
	psys->sound_status = get_sound_status();
	psys->audio_status = get_audio_status();
	psys->vibration_status = get_vibration_status();
	psys->voltage_status = get_voltage_status();
	psys->RSSI_status = get_rssi_status();
	psys->video_status = get_video_status();
	psys->call_status = get_call_status();
	psys->DNet_status = get_dnet_status();
//	psys->CPU_frequency = 	get_cpu_frequency(), freqbuf in get_resource_info()
//	psys->app_CPU_usage = 	app_cpu_usage in get_resource_info()
//	psys->CPU_load	cpuload in get_resource_info()
//	psys->virtual_memory = virtual in get_resource_info()
//	psys->resident_memory = resident in get_resource_info()
//	psys->shared_memory = shared in get_resource_info()
//	psys->PSS_memory = pss in get_resource_info()
//	psys->total_alloc_size = get_total_alloc_size();
//	psys->system_memory_total = get_system_total_memory(), sysmemtotal in get_resource_info();
//	psys->system_memory_used = update_system_memory_data(), sysmemused in get_resource_info();
	psys->total_used_drive = get_total_used_drive();
//	psys->count_of_threads = thread in get_resource_info()
//	psys->thread_load = thread_load in get_resource_info()
//	psys->count_of_processes	 = procNode* proc in get_resource_info() but no count

	return 0;
};
*/


/* int fill_data_msg_head (struct msg_data_t *data, uint32_t msgid, uint32_t seq, uint32_t len) */
/* { */
/* 	data->id = msgid; */
/* 	data->seq_num = seq; // TODO fill good value */
/* 	/\* gettimeofday(&data->time, NULL); *\/ */
/* 	data->len = len; */
/* 	return 0; */
/* } */


/* //allocate memory, need free!!! */
/* int gen_message_terminate(struct msg_data_t *data, uint32_t id) */
/* { */
/* 	char *p; */
/* 	fill_data_msg_head(data,NMSG_TERMINATE, 0, sizeof(uint32_t) ); */
/* 	// TODO fill good value */
/* 	data->payload = malloc(data->len); */

/* 	p = data->payload; */
/* 	pack_int(p,id); */
/* } */

//allocate memory, need free!!!
/* int gen_message_error(struct msg_data_t *data, const char * err_msg) */
/* { */
/* 	char *p; */
/* 	fill_data_msg_head(data, NMSG_ERROR, 0, strlen(err_msg)+1 ); */
/* 	data->payload = malloc(strlen(err_msg)+1); */

/* 	p = data->payload; */
/* 	pack_str(p,err_msg); */
/* 	return 0; */
/* } */

/* //allocatr memory, need free!!! */
/* int gen_message_event(struct msg_data_t *data, */
/* 		      struct input_event *events, */
/* 		      uint32_t events_count, */
/* 		      uint32_t id) */
/* { */
/* 	uint32_t i = 0; */
/* 	uint32_t payload_len = */
/* 		events_count * sizeof(*events)+ */
/* 		sizeof(payload_len); */
/* 	char *p; */



/* 	fill_data_msg_head(data, NMSG_RECORD, 0, payload_len ); */

/* 	data->payload = malloc(payload_len); */

/* 	p = data->payload; */
/* 	pack_int(p,events_count); */
/* 	for (i=0; i<events_count; i++){ */
/* 		pack_int32(p,id); */
/* 		pack_int32(p,events[i].type); */
/* 		pack_int32(p,events[i].code); */
/* 		pack_int32(p,events[i].value); */
/* 	} */

/* 	return 0; */
/* } */

int reset_data_msg(struct msg_data_t *data)
{

	free(data->payload);
	memset(data,0,sizeof(*data));

	return 0;
}

