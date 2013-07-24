/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
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
 * - Samsung RnD Institute Russia
 *
 */


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <sys/sysinfo.h>

#include "da_protocol.h"
#include "daemon.h"
#include "sys_stat.h"
#include "transfer_thread.h"
#include "elf.h"
#include "ioctl_commands.h"
#include "debug.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



void inline free_msg(struct msg_t *msg)
{
	free(msg);
}
static uint32_t msg_size_with_out_replays = 0;

struct prof_session_t prof_session;

static void print_app_info( struct app_info_t *app_info);
static void print_conf(struct conf_t * conf);
//DEBUG FUNCTIONS
#define dstr(x) #x

#define check_and_return(par,check) if ( par == check ) {return dstr(check);}
#define check_2(a1,a2) check_and_return(ID,a1) else check_and_return(ID,a2)
#define check_4(a1,a2,a3,a4) check_2(a1,a2) else check_2(a3,a4)
#define check_8(a1,a2,a3,a4,a5,a6,a7,a8) check_4(a1,a2,a3,a4) else check_4(a5,a6,a7,a8)

#define check_all(a1, ...) check_and_return(ID,a1) //#else check_all(__VA_ARGS__)
char* msg_ID_str ( enum HostMessageT ID)
{
	check_8(
		NMSG_KEEP_ALIVE,
		NMSG_START,
		NMSG_STOP,
		NMSG_CONFIG,
		0,
		NMSG_BINARY_INFO,
		NMSG_GET_TARGET_INFO,
		NMSG_SWAP_INST_ADD
		)
	else
	check_8(
		NMSG_SWAP_INST_REMOVE,
		NMSG_KEEP_ALIVE_ACK,
		NMSG_START_ACK,
		NMSG_STOP_ACK,

		NMSG_CONFIG_ACK,
		NMSG_BINARY_INFO_ACK,
		NMSG_SWAP_INST_ACK,
		NMSG_GET_TARGET_INFO_ACK
		)
	else
	check_8(
		NMSG_SWAP_INST_ADD_ACK,
		NMSG_SWAP_INST_REMOVE_ACK,
		NMSG_PROCESS_INFO,
		NMSG_TERMINATE,

		NMSG_ERROR,
		NMSG_SAMPLE,
		NMSG_SYSTEM,
		NMSG_IMAGE
		)
	else
	check_8(
		NMSG_RECORD,
		NMSG_FUNCTION_ENTRY,
		NMSG_FUNCTION_EXIT,
		NMSG_CONTEXT_SWITCH_ENTRY,

		NMSG_CONTEXT_SWITCH_EXIT,
		NMSG_PROBE,
		NMSG_PROBE_MEMORY,
		NMSG_PROBE_UICONTROL
		)
	else
	check_8(
		NMSG_PROBE_UIEVENT,
		NMSG_PROBE_RESOURCE,
		NMSG_PROBE_LIFECYCLE,
		NMSG_PROBE_SCREENSHOT,

		NMSG_PROBE_SCENE,
		NMSG_PROBE_THREAD,
		NMSG_PROBE_CUSTOM,
		NMSG_PROBE_SYNC
	) else
	return "HZ";
}


static char *msgErrStr(enum ErrorCode err)
{
	switch (err) {
	case ERR_NO:
		return "success";
	case ERR_LOCKFILE_CREATE_FAILED:
		return "lock file create failed";
	case ERR_ALREADY_RUNNING:
		return "already running";
	case ERR_INITIALIZE_SYSTEM_INFO_FAILED:
		return "initialize system info failed";
	case ERR_HOST_SERVER_SOCKET_CREATE_FAILED:
		return "host server socket create failed";
	case ERR_TARGET_SERVER_SOCKET_CREATE_FAILED:
		return "target server socket create failed";
	case ERR_SIGNAL_MASK_SETTING_FAILED: //TODO del (old parametr)
		return "ERR SIGNAL MASK SETTING FAILED";
	case ERR_WRONG_MESSAGE_FORMAT:
		return "wrong message format";
	case ERR_WRONG_MESSAGE_TYPE:
		return "wrong message type";
	case ERR_WRONG_MESSAGE_DATA:
		return "wrong message data";
	case ERR_CANNOT_START_PROFILING:
		return "cannot start profiling";
	case ERR_SERV_SOCK_CREATE:
		return "server socket creation failed (written in /tmp/da.port file)";
	case ERR_SERV_SOCK_BIND:
		return "server socket bind failed (written in /tmp/da.port file)";
	case ERR_SERV_SOCK_LISTEN:
		return "server socket listen failed (written in /tmp/da.port file)";
	case ERR_UNKNOWN:
	default:
		return "unknown error";
	}
	return "unknown error";
}


#define print_feature(f,in,to,delim) if (f & in)\
								{\
									sprintf(to, dstr(f) delim );\
									to+=strlen( dstr(f) delim );\
								}
#define print_feature_a(f) print_feature(f,feature,to,", ")
void feature_code_str(uint32_t feature, char * to)
{
	print_feature_a(FL_CPU);
	print_feature_a(FL_MEMORY);
	print_feature_a(FL_FUNCTION_PROFILING);
	print_feature_a(FL_MEMORY_ALLCATION_PROBING);
	print_feature_a(FL_FILE_API_PROBING);
	print_feature_a(FL_THREAD_API_PROBING);
	print_feature_a(FL_OSP_UI_API_PROBING);
	print_feature_a(FL_SCREENSHOT);
	print_feature_a(FL_USER_EVENT);
	print_feature_a(FL_RECORDING);
	print_feature_a(FL_SYSTCALL_FILE);
	print_feature_a(FL_SYSTCALL_IPC);
	print_feature_a(FL_SYSTCALL_PROCESS);
	print_feature_a(FL_SYSTCALL_SIGNAL);
	print_feature_a(FL_SYSTCALL_NETWORK);
	print_feature_a(FL_SYSTCALL_DESC);
	print_feature_a(FL_CONTEXT_SWITCH);
	print_feature_a(FL_NETWORK_API_PROBING);
	print_feature_a(FL_OPENGL_API_PROBING);

}


//PARSE FUNCTIONS
inline uint32_t get_avail_msg_size(struct msg_buf_t *msg)
{
	return (uint32_t)(msg->end - msg->cur_pos);
}

inline uint32_t get_msg_cur_size(struct msg_buf_t *msg)
{
	return (uint32_t) (msg->cur_pos - msg->payload);
}

static int parse_string(struct msg_buf_t *msg, char **str)
{
	parse_deb("size = %d\n", get_avail_msg_size(msg));
	int len = strlen(msg->cur_pos) + 1;

	if (get_avail_msg_size(msg) < len)
		return 0;

	*str = strdup(msg->cur_pos);
	parse_deb("<%s>\n",*str);
	msg->cur_pos += len;
	return 1;
}

static int parse_int32(struct msg_buf_t *msg, uint32_t *val)
{
	parse_deb("size = %d\n", get_avail_msg_size(msg));
	if (get_avail_msg_size(msg) < sizeof(*val))
		return 0;
	*val = *(uint32_t *)msg->cur_pos;
	msg->cur_pos += sizeof(uint32_t);


	parse_deb("<%d><0x%08X>\n",*val,*val);
	return 1;
}

static int parse_int64(struct msg_buf_t *msg, uint64_t *val)
{
	parse_deb("size = %d\n", get_avail_msg_size(msg));
	if (get_avail_msg_size(msg) < sizeof(*val))
		return 0;

	*val = *(uint64_t *)msg->cur_pos;

	parse_deb("<%llu><0x%016llX>\n",*val,*val);
	msg->cur_pos += sizeof(uint64_t);
	return 1;
}

static int parse_app_info(struct msg_buf_t *msg,
			       struct app_info_t *app_info)
{

	parse_deb("parse_app_info\n");
	if (!parse_int32(msg, &app_info->app_type)){
		LOGE("app type parsing error\n");
		return 0;
	}

	if (!parse_string(msg, &app_info->app_id)){
		LOGE("app id parsing error\n");
		return 0;
	}

	if (!parse_string(msg, &app_info->exe_path)) {
		LOGE("app info parsing error\n");
		return 0;
	}

//	print_app_info(app_info);

	return 1;
}

static int parse_conf(struct msg_buf_t *msg, struct conf_t *conf)
{

	parse_deb("parse_conf\n");
	if (!parse_int64(msg, &conf->use_features0)) {
		LOGE("use features parsing error\n");
		return 0;
	}

	if (!parse_int64(msg, &conf->use_features1)) {
		LOGE("use features parsing error\n");
		return 0;
	}

	if (!parse_int32( msg, &conf->system_trace_period)) {
		LOGE("system trace period parsing error\n");
		return 0;
	}

	if (!parse_int32( msg, &conf->data_message_period)) {
		LOGE("data message period parsing error\n");
		return 0;
	}
	//print_conf(conf);
	return 1;
}

static int parse_us_inst_func(struct msg_buf_t *msg , struct us_func_inst_t * dest)
{

	if (!parse_int64(msg, &(dest->func_addr))) {
		LOGE("func addr parsing error\n");
		return 0;
	}

	if (!parse_string(msg, &dest->args)) {
		LOGE("args format parsing error\n");
		return 0;
	}
	return 1;

}

static int parse_func_inst_list(struct msg_buf_t *msg,
			    uint32_t *num,
			    struct us_func_inst_t ** us_func_inst_list)
{
	uint32_t i = 0;
	if (!parse_int32( msg, num)) {
		LOGE("func num parsing error\n");
		return 0;
	}
	//parse user space function list

	parse_deb("us_func_inst_list size = %d * %d\n",(*num) , (int)sizeof(**us_func_inst_list) );
	*us_func_inst_list =
		(struct us_func_inst_t *)
		malloc( (*num) * sizeof(**us_func_inst_list) );
	if (!*us_func_inst_list){
		LOGE("func alloc error\n");
		return 0;
	};

	for ( i = 0; i < *num; i++){
		if (!parse_us_inst_func( msg, &((* us_func_inst_list)[i]))){
			LOGE("parse us inst func #%d failed\n", i + 1);
			return 0;
		}
	}

	return 1;
}

static int parse_us_inst_lib(struct msg_buf_t *msg, struct us_lib_inst_t * dest)
{

	if (!parse_string(msg, &(dest)->bin_path)) {
		LOGE("bin path parsing error\n");
		return 0;
	}

	if (!parse_func_inst_list(msg, &dest->func_num, &dest->us_func_inst_list)) {
		LOGE("funcs parsing error\n");
		return 0;
	}
	return 1;

}

static int parse_lib_inst_list(struct msg_buf_t *msg,
			    uint32_t *num,
			    struct us_lib_inst_t ** us_lib_inst_list)
{
	uint32_t i = 0;
	if (!parse_int32( msg, num)) {
		LOGE("lib num parsing error\n");
		return 0;
	}

	parse_deb("lib_list size = %d\n", (*num) * (int)sizeof(**us_lib_inst_list) );
	*us_lib_inst_list =
		(struct us_lib_inst_t *)
		malloc( (*num) * sizeof(**us_lib_inst_list) );
	if (!*us_lib_inst_list){
		LOGE("lib alloc error\n");
		return 0;
	};
	for ( i = 0; i < *num; i++){
		if (!parse_us_inst_lib( msg, &( (*us_lib_inst_list)[i] ) )){
			LOGE("parse is inst lib #%d failed\n", i + 1);
			return 0;
		}
	}
	return 1;
}

static int parse_app_inst(struct msg_buf_t *msg,
			    struct app_inst_t *app_inst)
{
	if (!parse_int32( msg, &app_inst->app_type)) {
		LOGE("app type parsing error\n");
		return 0;
	}
	if (!parse_string( msg, &app_inst->app_id)) {
		LOGE("app id parsing error\n");
		return 0;
	}
	if (!parse_string( msg, &app_inst->exec_path)) {
		LOGE("exec path parsing error\n");
		return 0;
	}
	if (!parse_func_inst_list( msg, &app_inst->func_num , &(app_inst->us_func_inst_list))) {
		LOGE("funcs parsing error\n");
		return 0;
	}

	parse_deb(">=%04X : %s, %s, %d\n",
	     app_inst->app_type, app_inst->app_id, app_inst->exec_path , num);

	if (!parse_lib_inst_list( msg, &app_inst->lib_num , &app_inst->us_lib_inst_list)) {
		LOGE("libs parsing error\n");
		return 0;
	}

	return 1;
}

int parse_user_space_inst(struct msg_buf_t *msg,
			    struct user_space_inst_t *user_space_inst)
{
	parse_deb("parse_user_space_inst\n");
	uint32_t num = 0 , i = 0;
	struct app_inst_t * list = 0;

	if (!parse_int32 ( msg, &num )) {
		LOGE("app num parsing error\n");
		return 0;
	}

	parse_deb("%d * %d\n",(int) sizeof(*(user_space_inst->app_inst_list)), num);
	if ( num != 0 ) {
		list = (struct app_inst_t *) malloc (
			sizeof(*(user_space_inst->app_inst_list)) * num);
		if ( !list ) {
			LOGE("apps alloc error\n");
			return 0;
		};

		for ( i = 0; i < num; i++){
			if (!parse_app_inst( msg, &(list[i]) )){
				LOGE("parse app inst #%d failed\n", i + 1);
				return 0;
			}
		};

		user_space_inst->app_num = num;
		user_space_inst->app_inst_list = list;
	}
	return 1;
}

//REPLAY EVENTS PARSE
static int parse_timeval(struct msg_buf_t *msg, struct timeval *tv)
{
	uint32_t nsec = 0;

	parse_deb("time\n");

	// FIXME: is sec/usec order correct?
	if (!parse_int32(msg, &nsec)) {
		LOGE("usec parsing error\n");
		return 0;
	}
	tv->tv_usec = nsec / 1000;

	if (!parse_int32(msg, (uint32_t *)&tv->tv_sec)) {
		LOGE("sec parsing error\n");
		return 0;
	}

	return 1;
}

static int parse_replay_event(struct msg_buf_t *msg,
				    struct replay_event_t *re)
{

	if (!parse_timeval(msg, &re->ev.time)) {
		LOGE("time parsing error\n");
		return 0;
	}

	if (!parse_int32(msg, &re->id)) {
		LOGE("id parsing error\n");
		return 0;
	}

	if (!parse_int32(msg, (uint32_t *)&re->ev.type)) {
		LOGE("type parsing error\n");
		return 0;
	}

	if (!parse_int32(msg, (uint32_t *)&re->ev.code)) {
		LOGE("code parsing error\n");
		return 0;
	}

	if (!parse_int32(msg, (uint32_t *)&re->ev.value)) {
		LOGE("value parsing error\n");
		return 0;
	}

	return 1;
}

void reset_replay_event_seq(struct replay_event_seq_t *res)
{
	res->enabled = 0;
	res->tv = (struct timeval){0, 0};
	res->event_num = 0;
	free(res->events);
}

static int parse_replay_event_seq(struct msg_buf_t *msg,
				    struct replay_event_seq_t *res)
{
	LOGI("parse_replay_event_seq\n");

	int i = 0;
	parse_deb("REPLAY\n");
	if (!parse_int32(msg, &res->enabled)) {
		LOGE("enabled parsing error\n");
		return 0;
	}

	if(res->enabled == 0){
		parse_deb("disable\n");
		return 1;
	}

	parse_deb("time main\n");
	if (!parse_timeval(msg, &res->tv)) {
		LOGE("time parsing error\n");
		return 0;
	}

	parse_deb("count\n");
	if (!parse_int32(msg, &res->event_num)) {
		LOGE("event num parsing error\n");
		return 0;
	}
	parse_deb("events num=%d\n", res->event_num);

	res->events = (struct replay_event_t *)malloc(res->event_num *
						      sizeof(*res->events));
	if (!res->events) {
		LOGE("events alloc error\n");
		return 0;
	}

	for (i = 0; i < res->event_num; i++) {
		parse_deb("sub_rep\n");
		if (!parse_replay_event(msg, &res->events[i])) {
			LOGE("event #%d parsing error\n", i + 1);
			free(res->events);
			res->event_num = 0;
			return 0;
		}
	}

	return 1;
}

//*REPLAY EVENT PARSE

static int parse_prof_session(struct msg_buf_t *msg,
			      struct prof_session_t *prof_session)
{
	LOGI("parse_prof_session\n");
	if (!parse_app_info(msg, &prof_session->app_info)) {
		LOGE("app info parsing error\n");
		return 1;
	}
	if (!parse_conf(msg, &prof_session->conf)) {
		LOGE("conf parsing error\n");
		return 1;
	}

	if (!parse_user_space_inst(msg, &prof_session->user_space_inst)) {
		LOGE("user space inst parsing error\n");
		return 1;
	}

	msg_size_with_out_replays = get_msg_cur_size(msg);
	if (!parse_replay_event_seq(msg, &prof_session->replay_event_seq)) {
		LOGE("replay parsing error\n");
		return 1;
	}

	//print_prof_session(prof_session);
	return 0;
}

int get_sys_mem_size(uint32_t *sys_mem_size){
	struct sysinfo info;
	sysinfo(&info);
	*sys_mem_size = info.totalram;
	return 0;
}

static int parse_msg_config(struct msg_buf_t * msg_payload,
			      struct conf_t * conf)
{
	if (!parse_conf(msg_payload, conf)) {
		LOGE("conf parsing error\n");
		return 0;
	}

	print_conf(conf);
	return 1;
}

static int parse_msg_binary_info(struct msg_buf_t * msg_payload,
			      struct app_info_t *app_info)
{
	if (!parse_app_info(msg_payload, app_info)) {
		LOGE("app info parsing error\n");
		return 0;
	}

	print_app_info(app_info);
	return 1;
}

static void init_parse_control(struct msg_buf_t *buf, struct msg_t *msg)
{
	LOGI("init parse control\n");
	buf->payload = msg->payload;
	buf->len = msg->len;
	buf->end = msg->payload + msg->len;
	buf->cur_pos = msg->payload;
	LOGI("init parse control done\n");
}

//This function concat 2 user space lists
// this function clean "from" pointer
static void concat_add_user_space_inst(struct user_space_inst_t *from,
								struct user_space_inst_t *to)
{
	struct app_inst_t *new_app_inst_list = NULL;
	uint32_t size;
	void *p;

	if (from->app_num == 0)
		return;

	new_app_inst_list = malloc((from->app_num + to->app_num) * sizeof(*new_app_inst_list));
	p = new_app_inst_list;

	size = from->app_num * sizeof(*new_app_inst_list);
	memcpy(p, from->app_inst_list, size);
	p +=size;

	size = to->app_num * sizeof(*new_app_inst_list);
	memcpy(p, to->app_inst_list, size);
	p +=size;

	free(to->app_inst_list);
	to->app_inst_list = new_app_inst_list;

	to->app_num += from->app_num;
	return;
}

static void cut_replay_events(struct msg_t *msg){

	LOGI("msg_size_with_out_replays = %d \n",msg_size_with_out_replays);
	msg->len = msg_size_with_out_replays;

}

static void reset_app_info(struct app_info_t *app_info)
{
	if (app_info->app_id != NULL)
		free(app_info->app_id);
	if (app_info->exe_path != NULL)
		free(app_info->exe_path);
	memset(app_info, 0, sizeof(*app_info));
}

static void reset_target_info(struct target_info_t *target_info)
{
	free(target_info->network_type);
}

static void reset_conf(struct conf_t *conf)
{
	memset(conf, 0, sizeof(*conf));
}

static void reset_func_inst_list(uint32_t func_num,
				 struct us_func_inst_t *funcs)
{
	int i = 0;

	for (i = 0; i < func_num; i++) {
		funcs[i].func_addr = 0;
		free(funcs[i].args);
	}
}

static void reset_lib_inst_list(uint32_t lib_num, struct us_lib_inst_t *libs)
{
	int i = 0;

	for (i = 0; i < lib_num; i++) {
		free(libs[i].bin_path);
		reset_func_inst_list(libs[i].func_num,
				     libs[i].us_func_inst_list);
	}
}

static void reset_app_inst(struct app_inst_t *app_inst)
{
	app_inst->app_type = 0;
	free(app_inst->app_id);
	free(app_inst->exec_path);
	reset_func_inst_list(app_inst->func_num,
			     app_inst->us_func_inst_list);
	app_inst->func_num = 0;
	free(app_inst->us_func_inst_list);
	reset_lib_inst_list(app_inst->lib_num, app_inst->us_lib_inst_list);
	app_inst->lib_num = 0;
	free(app_inst->us_lib_inst_list);
}

static void reset_user_space_inst(struct user_space_inst_t *us)
{
	int i = 0;

	for (i = 0; i < us->app_num; i++)
		reset_app_inst(&us->app_inst_list[i]);
	if (us->app_inst_list != NULL){
		free(us->app_inst_list);
		us->app_inst_list = NULL;
	}
	us->app_num = 0;
}

void reset_system_info(struct system_info_t *sys_info)
{
	if (sys_info->thread_load)
		free(sys_info->thread_load);
	if (sys_info->process_load)
		free(sys_info->process_load);
	if (sys_info->cpu_frequency)
		free(sys_info->cpu_frequency);
	if (sys_info->cpu_load)
		free(sys_info->cpu_load);
	memset(sys_info, 0, sizeof(*sys_info));
}

static void reset_prof_session(struct prof_session_t *prof_session)
{
	reset_app_info(&prof_session->app_info);
	reset_conf(&prof_session->conf);
	reset_user_space_inst(&prof_session->user_space_inst);
}

static struct msg_t *gen_binary_info_reply(struct app_info_t *app_info)
{
	uint32_t binary_type = get_binary_type(app_info->exe_path);
	char binary_path[PATH_MAX];
	struct msg_t *msg;
	char *p = NULL;
	uint32_t ret_id = ERR_NO;

	get_build_dir(binary_path, app_info->exe_path);

	if (binary_type == BINARY_TYPE_UNKNOWN) {
		LOGE("Binary is neither relocatable, nor executable\n");
		return NULL;
	}

	msg = malloc(sizeof(*msg) +
			      sizeof(ret_id) +
			      sizeof(binary_type) +
			      strlen(binary_path) + 1);
	if (!msg) {
		LOGE("Cannot alloc bin info msg\n");
		return NULL;
	}

	msg->id = NMSG_BINARY_INFO_ACK;
	p = msg->payload;

	pack_int(p, ret_id);
	pack_int(p, binary_type);
	pack_str(p, binary_path);

	msg->len = p - msg->payload;

	return msg;
}

static struct msg_t *gen_target_info_reply(struct target_info_t *target_info)
{
	struct msg_t *msg;
	char *p = NULL;
	uint32_t ret_id = ERR_NO;

	msg = malloc(sizeof(*msg) +
			      sizeof(ret_id) +
			      sizeof(*target_info) -
			      sizeof(target_info->network_type) +
			      strlen(target_info->network_type) + 1);
	if (!msg) {
		LOGE("Cannot alloc target info msg\n");
		free(msg);
		return NULL;
	}

	msg->id = NMSG_GET_TARGET_INFO_ACK;
	p = msg->payload;

	pack_int(p, ret_id);
	pack_int(p, target_info->sys_mem_size);
	pack_int(p, target_info->storage_size);
	pack_int(p, target_info->bluetooth_supp);
	pack_int(p, target_info->gps_supp);
	pack_int(p, target_info->wifi_supp);
	pack_int(p, target_info->camera_count);
	pack_str(p, target_info->network_type);
	pack_int(p, target_info->max_brightness);
	pack_int(p, target_info->cpu_core_count);

	msg->len = p - msg->payload;

	return msg;
}

static int send_reply(struct msg_t *msg)
{
	if (send(manager.host.control_socket,
		 msg, MSG_CMD_HDR_LEN + msg->len, MSG_NOSIGNAL) == -1) {
		LOGE("Cannot send reply : %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

static int sendACKToHost(enum HostMessageT resp, enum ErrorCode err_code,
			char *payload, int payload_size)
{
	if (manager.host.control_socket != -1)
	{
		struct msg_t *msg;
		uint32_t err = err_code;
		int loglen = sizeof(*msg) - sizeof(msg->payload) +
					 sizeof(err) + //return ID
					 payload_size;
		msg = malloc(loglen);
		char *p = msg->payload;

		//get ack message ID
		switch (resp) {
			case NMSG_KEEP_ALIVE:
				resp = NMSG_KEEP_ALIVE_ACK;
				break;
			case NMSG_START:
				resp = NMSG_START_ACK;
				break;
			case NMSG_STOP:
				resp = NMSG_STOP_ACK;
				break;
			case NMSG_CONFIG:
				resp = NMSG_CONFIG_ACK;
				break;
			case NMSG_BINARY_INFO:
				resp = NMSG_BINARY_INFO_ACK;
				break;
			case NMSG_GET_TARGET_INFO:
				resp = NMSG_GET_TARGET_INFO_ACK;
				break;
			case NMSG_SWAP_INST_ADD:
				resp = NMSG_SWAP_INST_ADD_ACK;
				break;
			case NMSG_SWAP_INST_REMOVE:
				resp = NMSG_SWAP_INST_REMOVE_ACK;
				break;
			default:
				//TODO report error
				free(msg);
				return 1;
		}

		//set message id
		msg->id = resp;
		//set payload lenth
		msg->len = payload_size + sizeof(err);
		//set return id
		//*(uint32_t *)p = err; p+=sizeof(err);
		pack_int(p, err)
		//copy payload data
		memcpy(p, payload, payload_size);

		LOGI("ACK (%s) errcode<%s> payload=%d; size=%d\n", msg_ID_str(resp),
				msgErrStr(err_code), (int)payload, payload_size);
		printBuf((char *)msg, loglen);

		if (send(manager.host.control_socket, msg,
			 loglen, MSG_NOSIGNAL) == -1) {
			LOGE("Cannot send reply: %s\n", strerror(errno));
			free(msg);
			return 1;
		}
		free(msg);
		return 0;
	}
	else
		return 1;
}

struct msg_t *gen_stop_msg(void){
	struct msg_t *res = malloc(sizeof(*res));
	memset(res, 0, sizeof(*res));
	res->id = NMSG_STOP;
	res->len = 0;
	return res;
}

enum ErrorCode stop_all(void)
{
	LOGI("entry\n");
	enum ErrorCode error_code = ERR_NO;
	struct msg_t *msg = gen_stop_msg();

	terminate_all();
	stop_profiling();

	if (msg == NULL){
		LOGE("cannot generate stop message\n");
		return ERR_UNKNOWN;
	} else {
		if (ioctl_send_msg(msg) != 0){
			LOGE("ioctl send filed\n");
			error_code = ERR_UNKNOWN;
		}
		free_msg(msg);
	}

	reset_prof_session(&prof_session);
	stop_transfer();

	LOGI("finished\n");
	return error_code;
}

int host_message_handler(struct msg_t *msg)
{
	struct app_info_t app_info;
	struct target_info_t target_info;
	struct msg_t *msg_reply;
	struct msg_buf_t msg_control;
	struct user_space_inst_t user_space_inst;
	struct conf_t conf;
	enum ErrorCode error_code;

	LOGI("MY HANDLE %s (%X)\n", msg_ID_str(msg->id), msg->id);
	init_parse_control(&msg_control, msg);

	switch (msg->id) {
	case NMSG_KEEP_ALIVE:
		sendACKToHost(msg->id, ERR_NO, 0, 0);
		break;
	case NMSG_START:
		if (parse_prof_session(&msg_control, &prof_session) != 0) {
			LOGE("prof session parsing error\n");
			sendACKToHost(msg->id, ERR_WRONG_MESSAGE_FORMAT, 0, 0);
			return -1;
		}

		// TODO: launch translator thread
		if (start_transfer() != 0) {
			LOGE("Cannot start transfer\n");
			return -1;
		}


		// TODO: apply_prof_session()

		//write to device
		cut_replay_events(msg);
		if (ioctl_send_msg(msg) != 0){
			LOGE("cannot send message to device\n");
			sendACKCodeToHost(MSG_NOTOK, ERR_CANNOT_START_PROFILING);
			return -1;
		}

		if (start_profiling() < 0) {
			LOGE("cannot start profiling\n");
			sendACKToHost(msg->id, ERR_CANNOT_START_PROFILING, 0, 0);
			return -1;
		}

		// TODO: start app launch timer


		// success
		sendACKToHost(msg->id, ERR_NO, 0, 0);
		break;
	case NMSG_STOP:
		error_code = stop_all();
		//send ack to host
		sendACKToHost(msg->id, error_code, 0, 0);
		break;
	case NMSG_CONFIG:
		error_code=ERR_NO;
		if (!parse_msg_config(&msg_control, &conf)) {
			LOGE("config parsing error\n");
			sendACKToHost(msg->id, ERR_WRONG_MESSAGE_FORMAT, 0, 0);
			return -1;
		}
		if (reconfigure(conf) != 0) {
			LOGE("Cannot change configuration\n");
			return -1;
		}
		//write to device
		if (ioctl_send_msg(msg) != 0){
			sendACKToHost(msg->id, ERR_UNKNOWN, 0, 0);
			return -1;
		}
		//send ack to host
		sendACKToHost(msg->id, ERR_NO, 0, 0);
		break;
	case NMSG_BINARY_INFO:
		if (!parse_msg_binary_info(&msg_control, &app_info)) {
			LOGE("binary info parsing error\n");
			sendACKToHost(msg->id, ERR_WRONG_MESSAGE_FORMAT, 0, 0);
			return -1;
		}
		msg_reply = gen_binary_info_reply(&app_info);
		if (!msg_reply) {
			sendACKToHost(msg->id, ERR_UNKNOWN, 0, 0);
			return -1;
		}

		if (send_reply(msg_reply) != 0) {
			LOGE("Cannot send reply\n");
		}

		reset_app_info(&app_info);
		free(msg_reply);
		break;
	case NMSG_SWAP_INST_ADD:
		if (!parse_user_space_inst(&msg_control,
					   &user_space_inst)) {
			LOGE("user space inst parsing error\n");
			sendACKToHost(msg->id, ERR_WRONG_MESSAGE_FORMAT, 0, 0);
			return -1;
		}
		// TODO: apply_prof_session()
		// warning concat_add_user_space_inst free user_space_inst
		// so, data will not be availible
		concat_add_user_space_inst(&user_space_inst, &prof_session.user_space_inst);
		//write to device
		if (ioctl_send_msg(msg) != 0){
			sendACKToHost(msg->id, ERR_UNKNOWN, 0, 0);
			return -1;
		}
		//send ack to host
		sendACKToHost(msg->id, ERR_NO, 0, 0);
		// TODO release user_space_inst
		break;
	case NMSG_SWAP_INST_REMOVE:
		if (!parse_user_space_inst(&msg_control,
					   &prof_session.user_space_inst)){
			sendACKToHost(msg->id, ERR_WRONG_MESSAGE_FORMAT, 0, 0);
			LOGE("user space inst parsing error\n");
			return -1;
		}
		if (ioctl_send_msg(msg) != 0){
			sendACKToHost(msg->id, ERR_UNKNOWN, 0, 0);
			return -1;
		}
		// TODO: apply_prof_session()
		sendACKToHost(msg->id, ERR_NO, 0, 0);
		break;
	case NMSG_GET_TARGET_INFO:
		fill_target_info(&target_info);
		msg_reply = gen_target_info_reply(&target_info);
		if (!msg_reply) {
			sendACKToHost(msg->id, ERR_UNKNOWN, 0, 0);
			return -1;
		}
		if (send_reply(msg_reply) != 0) {
			LOGE("Cannot send reply\n");
		}
		free(msg_reply);
		reset_target_info(&target_info);
		break;

	default:
		LOGE("unknown message %d <0x%08X>\n", msg->id, msg->id);
	}

	return 0;
}

// testing

static void print_app_info( struct app_info_t *app_info)
{
	LOGI("application info=\n");
	LOGI("\tapp_type=<%d><0x%04X>\n"
		 "\tapp_id=<%s>\n"
		 "\texe_path=<%s>\n",
		 app_info->app_type,
		 app_info->app_type,
		 app_info->app_id,
		 app_info->exe_path
	);
}

static void print_conf(struct conf_t * conf)
{
	char buf[1024];
	memset(&buf[0],0,1024);
	feature_code_str(conf->use_features0, buf);
	LOGI("conf = \n");
	LOGI("\tuse_features0 = 0x%016LX (%s)\n", conf->use_features0, buf);
	LOGI("\tuse_features1 = 0x%016LX (%s)\n", conf->use_features1, buf);
	LOGI(
		 "\tsystem_trace_period = %d ms\n"
		 "\tdata message period = %d ms\n",
		 conf->system_trace_period,
		 conf->data_message_period
		 );
}

void print_replay_event( struct replay_event_t *ev, uint32_t num, char *tab)
{
	LOGW("%s\t#%04d:time=0x%08X %08X, "
		" id=0x%08X,"
		" type=0x%08X,"
		" code=0x%08X,"
		" value=0x%08X\n",
		tab,num,
		(unsigned int)ev->ev.time.tv_sec,//timeval
		(unsigned int)ev->ev.time.tv_usec,//timeval
		ev->id,
		ev->ev.type,//u16
		ev->ev.code,//u16
		ev->ev.value//s32
		);
}

void print_replay_event_seq( struct replay_event_seq_t *event_seq)
{
	uint32_t i = 0;
	char *tab="\t";

	LOGI( "%senabled=0x%08X; "\
		"time_start=0x%08X %08X; "\
		"count=0x%08X\n",
		tab,event_seq->enabled,
		(unsigned int)event_seq->tv.tv_sec,
		(unsigned int)event_seq->tv.tv_usec,
		event_seq->event_num);
	for (i=0;i<event_seq->event_num;i++)
		print_replay_event(&event_seq->events[i], i+1, tab);

}
