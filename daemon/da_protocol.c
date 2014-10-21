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
#include "da_data.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

static pthread_mutex_t stop_all_mutex = PTHREAD_MUTEX_INITIALIZER;

static void inline free_msg(struct msg_t *msg)
{
	free(msg);
}

struct prof_session_t prof_session;

static void print_conf(struct conf_t *conf);
//DEBUG FUNCTIONS
#define dstr(x) #x
#define check_and_return(check) if ( ID == check ) {return dstr(check);}

char *msg_ID_str(enum HostMessageT ID)
{
	check_and_return(NMSG_KEEP_ALIVE);
	check_and_return(NMSG_START);
	check_and_return(NMSG_STOP);
	check_and_return(NMSG_CONFIG);
	check_and_return(NMSG_BINARY_INFO);
	check_and_return(NMSG_GET_TARGET_INFO);
	check_and_return(NMSG_SWAP_INST_ADD);
	check_and_return(NMSG_SWAP_INST_REMOVE);
	check_and_return(NMSG_GET_SCREENSHOT);
	check_and_return(NMSG_GET_PROCESS_ADD_INFO);

	check_and_return(NMSG_KEEP_ALIVE_ACK);
	check_and_return(NMSG_START_ACK);
	check_and_return(NMSG_STOP_ACK);
	check_and_return(NMSG_CONFIG_ACK);
	check_and_return(NMSG_BINARY_INFO_ACK);
	check_and_return(NMSG_SWAP_INST_ACK);
	check_and_return(NMSG_GET_TARGET_INFO_ACK);
	check_and_return(NMSG_SWAP_INST_ADD_ACK);
	check_and_return(NMSG_SWAP_INST_REMOVE_ACK);
	check_and_return(NMSG_GET_PROCESS_ADD_INFO_ACK);

	check_and_return(NMSG_PROCESS_INFO);
	check_and_return(NMSG_TERMINATE);
	check_and_return(NMSG_ERROR);
	check_and_return(NMSG_SAMPLE);
	check_and_return(NMSG_SYSTEM);
	check_and_return(NMSG_IMAGE);
	check_and_return(NMSG_RECORD);
	check_and_return(NMSG_FUNCTION_ENTRY);
	check_and_return(NMSG_FUNCTION_EXIT);
	check_and_return(NMSG_CONTEXT_SWITCH_ENTRY);
	check_and_return(NMSG_CONTEXT_SWITCH_EXIT);
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
}


#define print_feature(f,in,to,delim)					\
	if (f & in) {							\
		if (strlen(dstr(f) delim) + 1 < buflen ) {		\
			lenin = snprintf(to, buflen, dstr(f) delim );	\
			to += lenin;					\
			buflen -= lenin;				\
		} else { 						\
			goto err_exit;					\
		}							\
	}
#define print_feature_0(f) print_feature(f, feature0, to, ", \n\t")
static void feature_code_str(uint64_t feature0, uint64_t feature1, char *to,
			     uint32_t buflen)
{
	int lenin = 0;
	print_feature_0(FL_FUNCTION_PROFILING);
	print_feature_0(FL_MEMORY_ALLOC_PROBING);
	print_feature_0(FL_FILE_API_PROBING);
	print_feature_0(FL_THREAD_API_PROBING);
	print_feature_0(FL_OSP_UI_API_PROBING);
	print_feature_0(FL_SCREENSHOT);
	print_feature_0(FL_USER_EVENT);
	print_feature_0(FL_RECORDING);
	print_feature_0(FL_SYSTCALL_FILE);
	print_feature_0(FL_SYSTCALL_IPC);
	print_feature_0(FL_SYSTCALL_PROCESS);
	print_feature_0(FL_SYSTCALL_SIGNAL);
	print_feature_0(FL_SYSTCALL_NETWORK);
	print_feature_0(FL_SYSTCALL_DESC);
	print_feature_0(FL_CONTEXT_SWITCH);
	print_feature_0(FL_NETWORK_API_PROBING);
	print_feature_0(FL_OPENGL_API_PROBING);
	print_feature_0(FL_FUNCTION_SAMPLING);
	print_feature_0(FL_MEMORY_ALLOC_ALWAYS_PROBING);
	print_feature_0(FL_FILE_API_ALWAYS_PROBING);
	print_feature_0(FL_THREAD_API_ALWAYS_PROBING);
	print_feature_0(FL_OSP_UI_API_ALWAYS_PROBING);
	print_feature_0(FL_NETWORK_API_ALWAYS_PROBING);
	print_feature_0(FL_OPENGL_API_ALWAYS_PROBING);
	print_feature_0(FL_SYSTEM_CPU);
	print_feature_0(FL_SYSTEM_MEMORY);
	print_feature_0(FL_SYSTEM_PROCESS);
	print_feature_0(FL_SYSTEM_THREAD_LOAD);
	print_feature_0(FL_SYSTEM_PROCESSES_LOAD);
	print_feature_0(FL_SYSTEM_DISK);
	print_feature_0(FL_SYSTEM_NETWORK);
	print_feature_0(FL_SYSTEM_DEVICE);
	print_feature_0(FL_SYSTEM_ENERGY);

	goto exit;
err_exit:
	LOGE("Not enought mem to print\n");
exit:
	return;
}


//PARSE FUNCTIONS
static inline uint32_t get_avail_msg_size(struct msg_buf_t *msg)
{
	return (uint32_t)(msg->end - msg->cur_pos);
}

static inline uint32_t get_msg_cur_size(struct msg_buf_t *msg)
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

int parse_int8(struct msg_buf_t *msg, uint8_t *val)
{
	parse_deb("size = %d\n", get_avail_msg_size(msg));
	if (get_avail_msg_size(msg) < sizeof(*val))
		return 0;
	*val = *(uint8_t *)msg->cur_pos;
	msg->cur_pos += sizeof(uint8_t);

	parse_deb("<%d><0x%08X>\n", *val, *val);
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
	uint32_t dummy;

	if (!parse_timeval(msg, &re->ev.time)) {
		LOGE("time parsing error\n");
		return 0;
	}

	if (!parse_int32(msg, &re->id)) {
		LOGE("id parsing error\n");
		return 0;
	}

	/* FIXME ev.type, ev.code should be uint16_t */
	if (!parse_int32(msg, &dummy)) {
		LOGE("type parsing error\n");
		return 0;
	}
	re->ev.type = (uint16_t)dummy;

	if (!parse_int32(msg, &dummy)) {
		LOGE("code parsing error\n");
		return 0;
	}
	re->ev.code = (uint16_t)dummy;

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

	LOGI("Replay events: count = %u; total_size = %u\n",
	     res->event_num, res->event_num * sizeof(*res->events));

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

static void init_parse_control(struct msg_buf_t *buf, struct msg_t *msg)
{
	buf->payload = msg->payload;
	buf->len = msg->len;
	buf->end = msg->payload + msg->len;
	buf->cur_pos = msg->payload;
}

static void reset_target_info(struct target_info_t *target_info)
{
	return;
}

static void running_status_on(struct prof_session_t *prof_session)
{
	prof_session->running_status = 1;
}

static void running_status_off(struct prof_session_t *prof_session)
{
	prof_session->running_status = 0;
}

int check_running_status(const struct prof_session_t *prof_session)
{
	return prof_session->running_status;
}

static void reset_app_inst(struct user_space_inst_t *us_inst)
{
	free_data_list((struct data_list_t **)&us_inst->app_inst_list);
	us_inst->app_num = 0;
	us_inst->app_inst_list = NULL;
}

void reset_system_info(struct system_info_t *sys_info)
{
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
		GETSTRERROR(errno, buf);
		LOGE("Cannot send reply : %s\n", buf);
		return -1;
	}

	return 0;
}

static void write_msg_error(const char *err_str)
{
	struct msg_data_t *err_msg = gen_message_error(err_str);
	if (write_to_buf(err_msg) != 0)
		LOGE("write to buf fail\n");
	free_msg_data(err_msg);
}

static enum HostMessageT get_ack_msg_id(const enum HostMessageT id)
{
	switch (id) {
	case NMSG_KEEP_ALIVE:
		return NMSG_KEEP_ALIVE_ACK;
	case NMSG_START:
		return NMSG_START_ACK;
	case NMSG_STOP:
		return NMSG_STOP_ACK;
	case NMSG_CONFIG:
		return NMSG_CONFIG_ACK;
	case NMSG_BINARY_INFO:
		return NMSG_BINARY_INFO_ACK;
	case NMSG_GET_TARGET_INFO:
		return NMSG_GET_TARGET_INFO_ACK;
	case NMSG_SWAP_INST_ADD:
		return NMSG_SWAP_INST_ADD_ACK;
	case NMSG_SWAP_INST_REMOVE:
		return NMSG_SWAP_INST_REMOVE_ACK;
	case NMSG_GET_PROCESS_ADD_INFO:
		return NMSG_GET_PROCESS_ADD_INFO_ACK;
	default:
		LOGE("Fatal: unknown message ID [0x%X]\n", id);
		exit(EXIT_FAILURE);
	}
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

		resp = get_ack_msg_id(resp);

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
			GETSTRERROR(errno, buf);
			LOGE("Cannot send reply: %s\n", buf);
			free(msg);
			return 1;
		}
		free(msg);
		return 0;
	} else
		return 1;
}

static struct msg_t *gen_stop_msg(void)
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
		stop_replay();

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

static void stop_web_apps(void)
{
	const struct app_info_t *app_info;
	struct app_list_t *app = NULL;

	app_info = app_info_get_first(&app);
	while (app_info) {
		if (app_info->app_type == APP_TYPE_WEB)
			kill_app_web(app_info->app_id);
		app_info = app_info_get_next(&app);
	}
}

enum ErrorCode stop_all(void)
{
	enum ErrorCode error_code = ERR_NO;

	stop_web_apps();

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
	unsigned int len = strlen(ba->binpath) + 1;
	int i;
	*(uint32_t *) s = ba->type;
	s += sizeof(uint32_t);

	if (len)
		memcpy(s, ba->binpath, len);
	s += len;

	for (i = 0; i!= 16; ++i) {
		snprintf(s, 2, "%02x", ba->digest[i]);
		s += 2;
	}
	*s = '\0';

	return sizeof(uint32_t) + len + 2*16 + 1;
}

static void get_file_md5sum(md5_byte_t digest[16], const char *filename)
{
	md5_byte_t buffer[1024];
	ssize_t size;
	md5_state_t md5_state;
	int fd = open(filename, O_RDONLY);

	md5_init(&md5_state);
	if (fd >= 0) {
		while ((size = read(fd, buffer, sizeof(buffer))) > 0)
			md5_append(&md5_state, buffer, size);
		close(fd);
	} else {
		LOGW("File does not exists <%s>\n", filename);
	}

	md5_finish(&md5_state, digest);
}

static const char* basename(const char *filename)
{
	const char *p = strrchr(filename, '/');
	return p ? p + 1 : NULL;
}

/**
 * Checks whether it is Windows-style path or not.
 *
 * @return 1 if path is Windows-style one, 0 otherwise.
 */
static int check_windows_path(const char *path)
{
	size_t len;

	len = strlen(path);
	if (len > 3 && isalpha(path[0]) && !(strncmp(&(path[1]), ":\\", 2)))
		return 1;

	return 0;
}

static struct binary_ack* binary_ack_alloc(const char *filename)
{
	struct binary_ack *ba = malloc(sizeof(*ba));
	struct stat decoy;
	char builddir[PATH_MAX];
	char binpath[PATH_MAX];

	if (stat(filename, &decoy) == 0) {
		ba->type = get_binary_type(filename);

		get_build_dir(builddir, filename);

		if (builddir[0] != '\0')
			snprintf(binpath, sizeof(binpath), check_windows_path(builddir) ?
				 "%s\\%s" : "%s/%s", builddir, basename(filename) ?: "");
		else
			binpath[0] = '\0';

		ba->binpath = strdup(binpath);
		get_file_md5sum(ba->digest, filename);
	} else {
		ba->type = BINARY_TYPE_FILE_NOT_EXIST;
		ba->binpath = strdup(filename);
		memset(ba->digest, 0x00, sizeof(ba->digest));
	}

	return ba;
}

static int process_msg_binary_info(struct msg_buf_t *msg)
{
	uint32_t i, bincount;
	enum ErrorCode error_code = ERR_NO;

	printBuf(msg->cur_pos, msg->len);

	if (!parse_int32(msg, &bincount)) {
		LOGE("MSG_BINARY_INFO error: No binaries count\n");
		return -1;
	}

	struct binary_ack *acks[bincount];
	struct binary_ack *new;
	size_t total_size = 0;
	for (i = 0; i != bincount; ++i) {
		const char *str = parse_string_inplace(msg);
		if (!str) {
			LOGE("MSG_BINARY_INFO error: No enough binaries\n");
			return -1;
		}
		new = binary_ack_alloc(str);
		/* check for errors */
		if (new->type == BINARY_TYPE_FILE_NOT_EXIST) {
			error_code = ERR_WRONG_MESSAGE_DATA;
			LOGW("binary file not exists <%s>\n", str);
		}
		if (new->binpath[0] == '\0')
			LOGW("section '.debug_str' not found in <%s>\n", str);
		acks[i] = new;
		total_size += binary_ack_size(new);
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

	pack_int32(p, error_code);
	pack_int32(p, bincount);

	for (i = 0; i != bincount; ++i) {
		p += binary_ack_pack(p, acks[i]);
		binary_ack_free(acks[i]);
	}

	printBuf(msg_reply, msg_reply->len + sizeof(*msg_reply));
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
	struct msg_t *msg_reply = NULL;
	uint32_t serialized_time[2];

	//get start profiling time
	get_serialized_time(serialized_time);

	if (check_running_status(&prof_session) == 1) {
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

	//get time right before ioctl for more accurate start time value
	get_serialized_time(serialized_time);

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
	sendACKToHost(NMSG_START, err_code, (void *)&serialized_time,
		      sizeof(serialized_time));
	if (msg_reply != NULL)
		free(msg_reply);
	return -(err_code != ERR_NO);
}

int send_msg_to_sock(int sock, struct msg_target_t *msg)
{
	ssize_t ret;
	size_t n;
	int err = 0;

	n = sizeof(struct _msg_target_t) + msg->length;
	ret = send(sock, msg, n, MSG_NOSIGNAL);
	if (ret != n) {
		LOGE("fail to send data to socket(%d) n=%u, ret=%d\n",
		     sock, n, ret);
		err = 1;
	}

	return err;
}

int recv_msg_from_sock(int sock, struct msg_target_t *msg)
{
	ssize_t ret;

	ret = recv(sock, msg, MSG_HEADER_LEN, MSG_WAITALL);
	if (ret != MSG_HEADER_LEN)
		return 1;

	if (IS_PROBE_MSG(msg->type)) {
		struct msg_data_t *msg_data = (struct msg_data_t *)msg;
		size_t n = MSG_DATA_HDR_LEN - MSG_HEADER_LEN;

		ret = recv(sock, (char *)msg_data + MSG_HEADER_LEN,
			   n, MSG_WAITALL);
		if (ret != n)
			return 1;

		/* TODO: check msg_data->len */
		ret = recv(sock, msg_data->payload,
			   msg_data->len, MSG_WAITALL);

		if (ret != msg_data->len)
			return 1;

		return 0;
	}

	if (msg->length > 0) {
		/* TODO: check msg->length */
		ret = recv(sock, msg->data, msg->length, MSG_WAITALL);
		if (ret != msg->length)
			return 1;
	}

	return 0;
}

static int process_msg_get_screenshot(struct msg_buf_t *msg_control)
{
	uint32_t log_len;
	struct msg_target_t sendlog;
	enum ErrorCode err_code = ERR_UNKNOWN;

	// send config message to target process
	sendlog.type = MSG_CAPTURE_SCREEN;
	sendlog.length = 0;
	log_len = sizeof(sendlog.type) + sizeof(sendlog.length) + sendlog.length;

	if (target_send_msg_to_all(&sendlog) == 1)
		err_code = ERR_NO;

	return -(err_code != ERR_NO);
}

static char *get_process_cmd_line(uint32_t pid)
{
	char buf[MAX_FILENAME];
	int f;
	ssize_t count;

	snprintf(buf, sizeof(buf), "/proc/%u/cmdline", pid);
	f = open(buf, O_RDONLY);
	if (f != -1) {
		count = read(f, buf, sizeof(buf));
		if (count >= sizeof(buf))
			count = sizeof(buf) - 1;
		buf[count] = '\0';
		close(f);
	} else {
		LOGE("file not found <%s>\n", buf);
		buf[0] = '\0';
	}
	return strdup(buf);
}

static int process_msg_get_process_add_info(struct msg_buf_t *msg)
{
	uint32_t i, count, total_len;
	uint32_t *pidarr = NULL;
	char **cmd_line_arr = NULL;
	char *payload, *p;
	struct msg_target_t sendlog;
	enum ErrorCode err_code = ERR_UNKNOWN;

	/* get pid count */
	if (!parse_int32(msg, &count)) {
		LOGE("NMSG_GET_PROCESS_ADD_INFO error: No process count\n");
		err_code = ERR_WRONG_MESSAGE_DATA;
		goto send_fail;
	}

	/* alloc array for pids */
	pidarr = malloc(count * sizeof(*pidarr));
	cmd_line_arr = malloc(count * sizeof(*cmd_line_arr));
	if (pidarr == NULL) {
		LOGE("can not alloc pid array (%u)", count);
		goto send_fail;
	}
	if (cmd_line_arr == NULL) {
		LOGE("can not alloc cmd line array (%u)", count);
		goto send_fail;
	}

	/* parse all pids */
	for (i = 0; i != count; i++) {
		if (!parse_int32(msg, &pidarr[i])) {
			LOGE("can not parse pid #%u", i);
			goto send_fail;
		}
	}

	total_len = i * sizeof(*pidarr) + sizeof(count);
	for (i = 0; i != count; i++) {
		cmd_line_arr[i] = get_process_cmd_line(pidarr[i]);
		total_len += strlen(cmd_line_arr[i]) + 1;
	}

	payload = malloc(total_len);
	if (payload == NULL)
		goto send_fail;
	/* pack payload data */
	p = payload;
	pack_int32(p, count);
	for (i = 0; i != count; i++) {
		pack_int32(p, pidarr[i]);
		pack_str(p, cmd_line_arr[i]);
		free(cmd_line_arr[i]);
	}

	/* success */
	err_code = ERR_NO;
	goto send_ack;

send_fail:
	/* fail */
	total_len = 0;

send_ack:
	/* success */
	sendACKToHost(NMSG_GET_PROCESS_ADD_INFO, err_code, payload, total_len);

	/* free data */
	if (payload != NULL) {
		free(payload);
		payload = NULL;
	}

	if (pidarr != NULL) {
		free(pidarr);
		pidarr = NULL;
	}

	if (cmd_line_arr != NULL) {
		free(cmd_line_arr);
		cmd_line_arr = NULL;
	}

	return -(err_code != ERR_NO);
}

int host_message_handler(struct msg_t *msg)
{
	struct target_info_t target_info;
	struct msg_t *msg_reply = NULL;
	struct msg_buf_t msg_control;
	struct conf_t conf;
	enum ErrorCode error_code = ERR_NO;

	int target_index;
	struct msg_target_t sendlog;

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

		// TODO make it normally
		// Attention!!! convert feature to old format!!!
		uint64_t feature0 = *((uint64_t *)msg->payload);
		if (feature0 & FL_SYSTEM_ENERGY) {
			feature0 &= ~FL_SYSTEM_ENERGY;
			feature0 |= FL_SYSTEM_ENERGY_OLD;
		} else {
			feature0 &= ~FL_SYSTEM_ENERGY;
			feature0 &= ~FL_SYSTEM_ENERGY_OLD;
		}
		*((uint64_t *)msg->payload) = feature0;

		if (ioctl_send_msg(msg) != 0) {
			LOGE("ioctl send error\n");
			sendACKToHost(msg->id, ERR_UNKNOWN, 0, 0);
			return -1;
		}
		//send ack to host
		sendACKToHost(msg->id, ERR_NO, 0, 0);
		// send config message to target process
		sendlog.type = MSG_OPTION;
		sendlog.length = snprintf(sendlog.data, sizeof(sendlog.data),
					  "%llu",
					  (unsigned long long)
					  prof_session.conf.use_features0) + 1;
		target_send_msg_to_all(&sendlog);
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
	case NMSG_GET_SCREENSHOT:
		return process_msg_get_screenshot(&msg_control);
	case NMSG_GET_PROCESS_ADD_INFO:
		return process_msg_get_process_add_info(&msg_control);
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

static void print_conf(struct conf_t *conf)
{
	char buf[1024];
	memset(&buf[0], 0, 1024);
	feature_code_str(conf->use_features0, conf->use_features1, buf, sizeof(buf));
	LOGI("conf = \n");
	LOGI("\tuse_features = 0x%016LX : 0x%016LX \n(\t%s)\n",
	     conf->use_features0, conf->use_features1, buf);
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
