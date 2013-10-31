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
#include "da_inst.h"
#include "da_protocol_check.h"
#include "daemon.h"
#include "sys_stat.h"
#include "transfer_thread.h"
#include "elf.h"
#include "ioctl_commands.h"
#include "debug.h"
#include "md5.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static pthread_mutex_t stop_all_mutex = PTHREAD_MUTEX_INITIALIZER;

void inline free_msg(struct msg_t *msg)
{
	free(msg);
}

struct prof_session_t prof_session;

static void print_app_info(struct app_info_t *app_info);
static void print_conf(struct conf_t *conf);
//DEBUG FUNCTIONS
#define dstr(x) #x

#define check_and_return(par,check) if ( par == check ) {return dstr(check);}
#define check_2(a1,a2) check_and_return(ID,a1) else check_and_return(ID,a2)
#define check_4(a1,a2,a3,a4) check_2(a1,a2) else check_2(a3,a4)
#define check_8(a1,a2,a3,a4,a5,a6,a7,a8) check_4(a1,a2,a3,a4) else check_4(a5,a6,a7,a8)

#define check_all(a1, ...) check_and_return(ID,a1) //#else check_all(__VA_ARGS__)
char *msg_ID_str(enum HostMessageT ID)
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
void feature_code_str(uint32_t feature, char *to)
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

int parse_string(struct msg_buf_t *msg, char **str)
{
	parse_deb("size = %d\n", get_avail_msg_size(msg));
	int len = strlen(msg->cur_pos) + 1;

	if (get_avail_msg_size(msg) < len)
		return 0;

	*str = strdup(msg->cur_pos);
	parse_deb("<%s>\n", *str);
	msg->cur_pos += len;
	return 1;
}

static const char* parse_string_inplace(struct msg_buf_t *msg)
{
	const char *str = msg->cur_pos;
	int avail_size = get_avail_msg_size(msg);
	int len = strnlen(str, avail_size);

	/* Malformed string or exhaused buffer. Strlen is at least one byte
	 * less, that availiable space. If it is not, string is lacking
	 * terminating null char.
	 */

	if (len == avail_size)
		return NULL;

	msg->cur_pos += len + 1;
	return str;
}

int parse_string_no_alloc(struct msg_buf_t *msg, char *str)
{
	parse_deb("size = %d\n", get_avail_msg_size(msg));
	int len = strlen(msg->cur_pos) + 1;

	if (get_avail_msg_size(msg) < len)
		return 0;

	memcpy(str, msg->cur_pos, len);
	parse_deb("<%s>\n", str);
	msg->cur_pos += len;
	return 1;
}

int parse_int32(struct msg_buf_t *msg, uint32_t *val)
{
	parse_deb("size = %d\n", get_avail_msg_size(msg));
	if (get_avail_msg_size(msg) < sizeof(*val))
		return 0;
	*val = *(uint32_t *)msg->cur_pos;
	msg->cur_pos += sizeof(uint32_t);


	parse_deb("<%d><0x%08X>\n", *val, *val);
	return 1;
}


int parse_int64(struct msg_buf_t *msg, uint64_t *val)
{
	parse_deb("size = %d\n", get_avail_msg_size(msg));
	if (get_avail_msg_size(msg) < sizeof(*val))
		return 0;

	*val = *(uint64_t *)msg->cur_pos;

	parse_deb("<%llu><0x%016llX>\n", *val, *val);
	msg->cur_pos += sizeof(uint64_t);
	return 1;
}

static void strip_args(const char *cmd, char *path)
{
	char *bin_end = strchr(cmd, ' ');

	if (!bin_end) {
		strcpy(path, cmd);
	} else {
		size_t binname_len = bin_end - cmd;
		memcpy(path, cmd, binname_len);
		path[binname_len] = '\0';
	}
}

static int parse_app_info(struct msg_buf_t *msg,
			       struct app_info_t *app_info)
{
	char bin_path[MAX_FILENAME];

	//Application type
	parse_deb("parse_app_info\n");
	if (!parse_int32(msg, &app_info->app_type) ||
		!check_app_type(app_info->app_type))
	{
		LOGE("app type error\n");
		return 0;
	}
	//Application ID
	if (!parse_string(msg, &app_info->app_id) ||
		!check_app_id(app_info->app_type, app_info->app_id))
	{
		LOGE("app id parsing error\n");
		return 0;
	}
	//Applicaion exe path
	if (!parse_string(msg, &app_info->exe_path)) {
		LOGE("app info parsing error\n");
		return 0;
	}
	strip_args(app_info->exe_path, bin_path);
	if (!check_exec_path(bin_path)) {
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
		LOGE("use features0 error\n");
		return 0;
	}

	if (!parse_int64(msg, &conf->use_features1)) {
		LOGE("use features1 parsing error\n");
		return 0;
	}
	//Check features value
	if (!check_conf_features(conf->use_features0, conf->use_features1)) {
		LOGE("check features fail\n");
		return 0;
	}

	if (!parse_int32( msg, &conf->system_trace_period) ||
		!check_conf_systrace_period(conf->system_trace_period))
	{
		LOGE("system trace period error\n");
		return 0;
	}

	if (!parse_int32( msg, &conf->data_message_period) ||
		!check_conf_datamsg_period(conf->data_message_period))
	{
		LOGE("data message period error\n");
		return 0;
	}
	//print_conf(conf);
	return 1;
}

//REPLAY EVENTS PARSE
static int parse_timeval(struct msg_buf_t *msg, struct timeval *tv)
{
	uint32_t nsec = 0;

	parse_deb("time\n");

	if (!parse_int32(msg, (uint32_t *)&tv->tv_sec)) {
		LOGE("sec parsing error\n");
		return 0;
	}

	if (!parse_int32(msg, &nsec)) {
		LOGE("usec parsing error\n");
		return 0;
	}
	tv->tv_usec = nsec / 1000;

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
	if (res->event_num != 0)
		free(res->events);
	res->event_num = 0;
}

int parse_replay_event_seq(struct msg_buf_t *msg,
			   struct replay_event_seq_t *res)
{
	int i = 0;
	parse_deb("REPLAY\n");
	if (!parse_int32(msg, &res->enabled)) {
		LOGE("enabled parsing error\n");
		return 0;
	}

	if(res->enabled == 0) {
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

int get_sys_mem_size(uint32_t *sys_mem_size){
	struct sysinfo info;
	sysinfo(&info);
	*sys_mem_size = info.totalram;
	return 0;
}

static int parse_msg_config(struct msg_buf_t *msg_payload,
			      struct conf_t *conf)
{
	if (!parse_conf(msg_payload, conf)) {
		LOGE("conf parsing error\n");
		return 0;
	}

	print_conf(conf);
	return 1;
}

static int parse_msg_binary_info(struct msg_buf_t *msg_payload,
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
	buf->payload = msg->payload;
	buf->len = msg->len;
	buf->end = msg->payload + msg->len;
	buf->cur_pos = msg->payload;
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
	return;
}

static void reset_conf(struct conf_t *conf)
{
	memset(conf, 0, sizeof(*conf));
}

static void running_status_on(struct prof_session_t *prof_session)
{
	prof_session->running_status = 1;
}

static void running_status_off(struct prof_session_t *prof_session)
{
	prof_session->running_status = 0;
}

int check_running_status(struct prof_session_t *prof_session)
{
	return prof_session->running_status;
}

static void reset_app_inst(struct user_space_inst_t *us_inst)
{
	free_data_list((struct data_list_t **)&us_inst->app_inst_list);
	us_inst->app_num = 0;
	us_inst->app_inst_list = NULL;
}

static void reset_lib_inst(struct user_space_inst_t *us_inst)
{
	free_data_list((struct data_list_t **)&us_inst->lib_inst_list);
	us_inst->lib_num = 0;
	us_inst->lib_inst_list = NULL;
}

static void reset_user_space_inst(struct user_space_inst_t *us_inst)
{
	reset_app_inst(us_inst);
	reset_lib_inst(us_inst);
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

void init_prof_session(struct prof_session_t *prof_session)
{
	memset(prof_session, 0, sizeof(*prof_session));
}

static void reset_prof_session(struct prof_session_t *prof_session)
{
	reset_conf(&prof_session->conf);
	reset_user_space_inst(&prof_session->user_space_inst);
	reset_replay_event_seq(&prof_session->replay_event_seq);
	running_status_off(prof_session);
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

	pack_int32(p, ret_id);
	pack_int32(p, binary_type);
	pack_str(p, binary_path);

	msg->len = p - msg->payload;

	return msg;
}

static size_t str_array_getsize(const char **strings, size_t len)
{
	/*!
	 * Calculate about of memory to place array
	 * of \0 delimited strings
	 */
	size_t size = 0;
	unsigned int index;
	for (index = 0; index != len; ++index)
		size += strlen(strings[index]) + 1;
	return size;
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
		     strlen(target_info->network_type) + 1 +
		     sizeof(uint32_t) + /* devices count */
		     str_array_getsize(supported_devices_strings,
				       supported_devices_count));
	if (!msg) {
		LOGE("Cannot alloc target info msg\n");
		free(msg);
		return NULL;
	}

	msg->id = NMSG_GET_TARGET_INFO_ACK;
	p = msg->payload;

	pack_int32(p, ret_id);
	pack_int64(p, target_info->sys_mem_size);
	pack_int64(p, target_info->storage_size);
	pack_int32(p, target_info->bluetooth_supp);
	pack_int32(p, target_info->gps_supp);
	pack_int32(p, target_info->wifi_supp);
	pack_int32(p, target_info->camera_count);
	pack_str(p, target_info->network_type);
	pack_int32(p, target_info->max_brightness);
	pack_int32(p, target_info->cpu_core_count);
	pack_int32(p, supported_devices_count);
	p = pack_str_array(p, supported_devices_strings,
			   supported_devices_count);

	msg->len = p - msg->payload;

	return msg;
}

static int send_reply(struct msg_t *msg)
{
	printBuf((char *)msg, msg->len + sizeof (*msg));
	if (send(manager.host.control_socket,
		 msg, MSG_CMD_HDR_LEN + msg->len, MSG_NOSIGNAL) == -1) {
		LOGE("Cannot send reply : %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static void write_msg_error(const char *err_str)
{
	struct msg_data_t *err_msg = gen_message_error(err_str);
	write_to_buf(err_msg);
	free_msg_data(err_msg);
}

int sendACKToHost(enum HostMessageT resp, enum ErrorCode err_code,
			char *payload, int payload_size)
{
	if (manager.host.control_socket != -1) {
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
		pack_int32(p, err);
		//copy payload data
		memcpy(p, payload, payload_size);

		LOGI("ACK (%s) errcode<%s> payload=0x%08X; size=%d\n", msg_ID_str(resp),
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
	} else
		return 1;
}

struct msg_t *gen_stop_msg(void)
{
	struct msg_t *res = malloc(sizeof(*res));
	memset(res, 0, sizeof(*res));
	res->id = NMSG_STOP;
	res->len = 0;
	return res;
}


enum ErrorCode stop_all_no_lock(void)
{
	enum ErrorCode error_code = ERR_NO;
	struct msg_t *msg;

	// stop all only if it has not been called yet
	if (check_running_status(&prof_session)) {
		msg = gen_stop_msg();
		terminate_all();
		stop_profiling();

		if (msg == NULL) {
			LOGE("cannot generate stop message\n");
			error_code = ERR_UNKNOWN;
			goto stop_all_exit;
		} else {
			if (ioctl_send_msg(msg) != 0) {
				LOGE("ioctl send failed\n");
				error_code = ERR_UNKNOWN;
				free_msg(msg);
				goto stop_all_exit;
			}
			free_msg(msg);
		}

		// we reset only app inst no lib no confing reset
		reset_app_inst(&prof_session.user_space_inst);
		stop_transfer();
		running_status_off(&prof_session);
	} else
		LOGI("already stopped\n");

stop_all_exit:
	LOGI("finished: ret = %d\n", error_code);
	return error_code;
}

int stop_all_in_process(void)
{
	return (pthread_mutex_trylock(&stop_all_mutex) != 0);
}

void stop_all_done(void)
{
	pthread_mutex_unlock(&stop_all_mutex);
}

enum ErrorCode stop_all(void)
{
	enum ErrorCode error_code = ERR_NO;

	pthread_mutex_lock(&stop_all_mutex);
	error_code = stop_all_no_lock();
	pthread_mutex_unlock(&stop_all_mutex);

	return error_code;
}

struct binary_ack {
	uint32_t type;
	char *binpath;
	md5_byte_t digest[16];
};

static void binary_ack_free(struct binary_ack *ba)
{
	if (ba)
		free(ba->binpath);
	free(ba);
}

static size_t binary_ack_size(const struct binary_ack *ba)
{
	/* MD5 is 16 bytes, so 16*2 hex digits */
	return sizeof(uint32_t) + strlen(ba->binpath) + 1
		+ 2*16 + 1;
}

static size_t binary_ack_pack(char *s, const struct binary_ack *ba)
{
	unsigned int len = strlen(ba->binpath);
	int i;
	*(uint32_t *) s = ba->type;
	s += sizeof(uint32_t);

	if (len)
		memmove(s, ba->binpath, len);

	*(s += len) = '\0';
	s += 1;

	for (i = 0; i!= 16; ++i) {
		sprintf(s, "%02x", ba->digest[i]);
		s += 2;
	}
	*s = '\0';
	return sizeof(uint32_t) + len + 1 + 2*16 + 1;
}

static void get_file_md5sum(md5_byte_t digest[16], const char *filename)
{
	char buffer[1024];
	ssize_t size;
	md5_state_t md5_state;
	int fd = open(filename, O_RDONLY);

	md5_init(&md5_state);
	if (fd > 0)
		while ((size = read(fd, buffer, sizeof(buffer))) > 0)
			md5_append(&md5_state, buffer, size);

	md5_finish(&md5_state, digest);
	close(fd);
}

static const char* basename(const char *filename)
{
	const char *p = strrchr(filename, '/');
	return p ? p + 1 : NULL;
}
static struct binary_ack* binary_ack_alloc(const char *filename)
{
	struct binary_ack *ba = malloc(sizeof(*ba));
	char builddir[PATH_MAX];
	char binpath[PATH_MAX];

	ba->type = get_binary_type(filename);

	get_build_dir(builddir, filename);

	snprintf(binpath, sizeof(binpath), "%s/%s",
		 builddir, basename(filename) ?: "");

	ba->binpath = strdup(binpath);

	get_file_md5sum(ba->digest, filename);

	return ba;
}

static int process_msg_binary_info(struct msg_buf_t *msg)
{
	uint32_t i, bincount;

	if (!parse_int32(msg, &bincount)) {
		LOGE("MSG_BINARY_INFO error: No binaries count\n");
		return -1;
	}

	struct binary_ack *acks[bincount];
	size_t total_size = 0;
	for (i = 0; i != bincount; ++i) {
		const char *str = parse_string_inplace(msg);
		if (!str) {
			LOGE("MSG_BINARY_INFO error: No enough binaries\n");
			return -1;
		}
		acks[i] = binary_ack_alloc(str);
		total_size += binary_ack_size(acks[i]);
	}
	typedef uint32_t return_id;
	typedef uint32_t binary_ack_count;
	struct msg_t *msg_reply = malloc(sizeof(struct msg_t)
					 + sizeof(return_id)
					 + sizeof(binary_ack_count)
					 + total_size);
	char *p = msg_reply->payload;

	msg_reply->id = NMSG_BINARY_INFO_ACK;
	msg_reply->len = total_size + sizeof(return_id)
				    + sizeof(binary_ack_count);

	pack_int32(p, ERR_NO);
	pack_int32(p, bincount);

	for (i = 0; i != bincount; ++i) {
		p += binary_ack_pack(p, acks[i]);
		binary_ack_free(acks[i]);
	}

	int err = send_reply(msg_reply);
	free(msg_reply);
	return err;
}

static void get_serialized_time(uint32_t dst[2])
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	dst[0] = tv.tv_sec;
	dst[1] = tv.tv_usec * 1000;
}

static int process_msg_start(struct msg_buf_t *msg_control)
{
	enum ErrorCode err_code = ERR_CANNOT_START_PROFILING;
	struct msg_t *msg_reply;
	uint32_t serialized_time[2];

	if (check_running_status(&prof_session.conf) == 1) {
		LOGW("Profiling has already been started\n");
		goto send_ack;
	}

	if (!check_conf(&prof_session.conf)) {
		LOGE("wrong profile config\n");
		goto send_ack;
	}

	if (msg_start(msg_control, &prof_session.user_space_inst,
		      &msg_reply, &err_code) != 0) {
		LOGE("parse error\n");
		goto send_ack;
	}

	if (prepare_profiling() != 0) {
		LOGE("failed to prepare profiling\n");
		goto send_ack;
	}

	if (start_transfer() != 0) {
		LOGE("Cannot start transfer\n");
		goto send_ack;
	}

	if (ioctl_send_msg(msg_reply) != 0) {
		LOGE("cannot send message to device\n");
		goto send_ack;
	}

	running_status_on(&prof_session);

	if (start_profiling() < 0) {
		LOGE("cannot start profiling\n");
		if (stop_all() != ERR_NO) {
			LOGE("Stop failed\n");
			write_msg_error("Stop failed");
		}
		goto send_ack;
	}

	err_code = ERR_NO;
send_ack:
	get_serialized_time(&serialized_time);
	sendACKToHost(NMSG_START, err_code, (void *)&serialized_time,
		      sizeof(serialized_time));

	return -(err_code != ERR_NO);
}

int host_message_handler(struct msg_t *msg)
{
	struct app_info_t app_info;
	struct target_info_t target_info;
	struct msg_t *msg_reply = NULL;
	struct msg_buf_t msg_control;
	struct conf_t conf;
	enum ErrorCode error_code = ERR_NO;

	int target_index;
	ssize_t sendlen;
	msg_target_t sendlog;

	LOGI("MY HANDLE %s (%X)\n", msg_ID_str(msg->id), msg->id);
	init_parse_control(&msg_control, msg);

	switch (msg->id) {
	case NMSG_KEEP_ALIVE:
		sendACKToHost(msg->id, ERR_NO, 0, 0);
		break;
	case NMSG_START:
		return process_msg_start(&msg_control);
	case NMSG_STOP:
		sendACKToHost(msg->id, ERR_NO, 0, 0);
		if (stop_all() != ERR_NO) {
			LOGE("Stop failed\n");
			write_msg_error("Stop failed");
		}
		break;
	case NMSG_CONFIG:
		error_code = ERR_NO;
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
		if (ioctl_send_msg(msg) != 0) {
			LOGE("ioctl send error\n");
			sendACKToHost(msg->id, ERR_UNKNOWN, 0, 0);
			return -1;
		}
		//send ack to host
		sendACKToHost(msg->id, ERR_NO, 0, 0);
		// send config message to target process
		sendlog.type = MSG_OPTION;
		sendlog.length = sprintf(sendlog.data, "%lu",
				     (unsigned long int) prof_session.conf.use_features0);
		for (target_index = 0; target_index < MAX_TARGET_COUNT; target_index++)
		{
			if(manager.target[target_index].socket != -1)
			{
				if (0 > send(manager.target[target_index].socket, &sendlog,
					sizeof(sendlog.type) + sizeof(sendlog.length) + sendlog.length,
					     MSG_NOSIGNAL))
					LOGE("fail to send data to target index(%d)\n", target_index);
			}
		}
		break;
	case NMSG_BINARY_INFO:
		return process_msg_binary_info(&msg_control);
	case NMSG_SWAP_INST_ADD:
		if (msg_swap_inst_add(&msg_control, &prof_session.user_space_inst,
				      &msg_reply, &error_code) != 0) {
			LOGE("swap inst add\n");
			goto send_ack;
		}
		if (msg_reply != NULL)
			if (ioctl_send_msg(msg_reply) != 0) {
				error_code = ERR_UNKNOWN;
				LOGE("ioclt send error\n");
			}
		//send ack to host
		goto send_ack;
	case NMSG_SWAP_INST_REMOVE:
		if (msg_swap_inst_remove(&msg_control, &prof_session.user_space_inst,
					 &msg_reply, &error_code) != 0) {
			LOGE("swap inst remove\n");
			error_code = ERR_UNKNOWN;
			goto send_ack;
		}
		if (msg_reply != NULL) {
			if (ioctl_send_msg(msg_reply) != 0)
				error_code = ERR_UNKNOWN;
		} else {
			error_code = ERR_UNKNOWN;
		}
		goto send_ack;
	case NMSG_GET_TARGET_INFO:
		fill_target_info(&target_info);
		msg_reply = gen_target_info_reply(&target_info);
		if (!msg_reply) {
			LOGE("cannot generate reply message\n");
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

send_ack:
	sendACKToHost(msg->id, error_code, 0, 0);
	if (msg_reply != NULL)
		free(msg_reply);
	return (error_code == ERR_NO);
}

// testing

static void print_app_info(struct app_info_t *app_info)
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

static void print_conf(struct conf_t *conf)
{
	char buf[1024];
	memset(&buf[0], 0, 1024);
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

void print_replay_event(struct replay_event_t *ev, uint32_t num, char *tab)
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

void print_replay_event_seq(struct replay_event_seq_t *event_seq)
{
	uint32_t i = 0;
	char *tab = "\t";

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
