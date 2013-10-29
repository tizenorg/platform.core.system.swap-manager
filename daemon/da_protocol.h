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


#ifndef _DA_PROTOCOL_
#define _DA_PROTOCOL_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <linux/input.h>

enum HostMessageT {
NMSG_KEEP_ALIVE			=0x0001,
NMSG_START				=0x0002,
NMSG_STOP				=0x0003,
NMSG_CONFIG				=0x0004,
NMSG_BINARY_INFO		=0x0005,
NMSG_GET_TARGET_INFO	=0x0007,
NMSG_SWAP_INST_ADD		=0x0008,
NMSG_SWAP_INST_REMOVE	=0x0009,

NMSG_KEEP_ALIVE_ACK			=0x1001,
NMSG_START_ACK				=0x1002,
NMSG_STOP_ACK				=0x1003,
NMSG_CONFIG_ACK				=0x1004,
NMSG_BINARY_INFO_ACK		=0x1005,
NMSG_SWAP_INST_ACK			=0x1006,
NMSG_GET_TARGET_INFO_ACK	=0x1007,
NMSG_SWAP_INST_ADD_ACK		=0x1008,
NMSG_SWAP_INST_REMOVE_ACK	=0x1009,

NMSG_PROCESS_INFO			=0x0001,	//	target process info
NMSG_TERMINATE				=0x0002,	//terminate
NMSG_ERROR					=0x0003,	//error message
NMSG_SAMPLE					=0x0004,	//N	10ms
NMSG_SYSTEM					=0x0005,	//N	10~1000ms	DaData, start sending immediately after start message from host, first system message time is tracing start time
NMSG_IMAGE					=0x0006,	//N	irregular	image
NMSG_RECORD					=0x0007,	//N	irregular	replay event
NMSG_FUNCTION_ENTRY			=0x0008,	//N	irregular	swap instrumentation, Instrumented functions by AppInst and LibInst
NMSG_FUNCTION_EXIT			=0x0009,	//N	irregular	swap instrumentation, Instrumented functions by AppInst and LibInst
NMSG_CONTEXT_SWITCH_ENTRY	=0x0010,	//N	irregular	swap instrumentation for kernel
NMSG_CONTEXT_SWITCH_EXIT	=0x0011,	//N	irregular	swap instrumentation for kernel

NMSG_PROBE					=0x0100,	//N	irregular	resource log
NMSG_PROBE_MEMORY			=0x0101,	//N	irregular	resource log
NMSG_PROBE_UICONTROL		=0x0102,	//N	irregular	resource log
NMSG_PROBE_UIEVENT			=0x0103,	//N	irregular	resource log
NMSG_PROBE_RESOURCE			=0x0104,	//N	irregular	resource log
NMSG_PROBE_LIFECYCLE		=0x0105,	//N	irregular	resource log
NMSG_PROBE_SCREENSHOT		=0x0106,	//N	irregular	resource log
NMSG_PROBE_SCENE			=0x0107,	//N	irregular	resource log
NMSG_PROBE_THREAD			=0x0108,	//N	irregular	resource log
NMSG_PROBE_CUSTOM			=0x0109,	//N	irregular	resource log
NMSG_PROBE_SYNC				=0x0110	//N	irregular	resource log


};
#define MSG_MAX_NUM NMSG_SWAP_INST_REMOVE

enum ErrorCode{
	ERR_NO									=0,		//success
	ERR_LOCKFILE_CREATE_FAILED				=-101,	//lock file create failed
	ERR_ALREADY_RUNNING						=-102,	//already running
	ERR_INITIALIZE_SYSTEM_INFO_FAILED		=-103,	//initialize system info failed
	ERR_HOST_SERVER_SOCKET_CREATE_FAILED	=-104,	//host server socket create failed
	ERR_TARGET_SERVER_SOCKET_CREATE_FAILED	=-105,	//target server socket create failed

	ERR_SIGNAL_MASK_SETTING_FAILED			=-106, //TODO del (old parametr)

	ERR_WRONG_MESSAGE_FORMAT				=-201,	//wrong message format
	ERR_WRONG_MESSAGE_TYPE					=-202,	//wrong message type
	ERR_WRONG_MESSAGE_DATA					=-203,	//wrong message data
	ERR_CANNOT_START_PROFILING				=-204,	//cannot start profiling
	ERR_SERV_SOCK_CREATE					=-900,	//server socket creation failed (written in /tmp/da.port file)
	ERR_SERV_SOCK_BIND						=-901,	//server socket bind failed (written in /tmp/da.port file)
	ERR_SERV_SOCK_LISTEN					=-902,	//server socket listen failed (written in /tmp/da.port file)
	ERR_UNKNOWN							=-999	//unknown error
};

enum feature_code{
	FL_RESERVED1                = 0x00000001,
	FL_RESERVED2                = 0x00000002,
	FL_FUNCTION_PROFILING       = 0x00000004, //On/Off the UserSpaceInst
	FL_MEMORY_ALLCATION_PROBING = 0x00000008, //memory allocation API (glibc)
	FL_FILE_API_PROBING         = 0x00000010, //file API (glibc, OSP)
	FL_THREAD_API_PROBING       = 0x00000020, //thread API (glibc, OSP)
	FL_OSP_UI_API_PROBING       = 0x00000040, //UI API (OSP)
	FL_SCREENSHOT               = 0x00000080, //Screenshot
	FL_USER_EVENT               = 0x00000100, //events of Touch, Gesture, Orientation, Key
	FL_RECORDING                = 0x00000200, //recording the user event
	FL_SYSTCALL_FILE            = 0x00000400, //File operation syscalls tracing
	FL_SYSTCALL_IPC             = 0x00000800, //IPC syscall tracing
	FL_SYSTCALL_PROCESS         = 0x00001000, //Process syscalls tracing
	FL_SYSTCALL_SIGNAL          = 0x00002000, //Signal syscalls tracing
	FL_SYSTCALL_NETWORK         = 0x00004000, //Network syscalls tracing
	FL_SYSTCALL_DESC            = 0x00008000, //Descriptor syscalls tracing
	FL_CONTEXT_SWITCH           = 0x00010000, //Context switch tracing
	FL_NETWORK_API_PROBING      = 0x00020000, //network API (glibc, OSP, libsoap, openssl)
	FL_OPENGL_API_PROBING       = 0x00040000, //openGL API
	FL_FUNCTION_SAMPLING        = 0x00080000, //Function sampling
	FL_CPU                      = 0x00100000, //CPU core load, frequency
	FL_PROCESSES                = 0x00200000, //Process load
	FL_MEMORY                   = 0x00400000, //Process size(VSS, PSS. RSS), heap usage(application, library), physical memory in use
	FL_DISK                     = 0x00800000,
	FL_NETWORK                  = 0x01000000,
	FL_DEVICE                   = 0x02000000,
	FL_ENERGY                   = 0x04000000,
	FL_RESERVED3                = 0x08000000,

	FL_ALL_FEATURES             = 0xFFFFFFFF &
	                              (~FL_RESERVED1) &
	                              (~FL_RESERVED2) &
	                              (~FL_RESERVED3)
};

#define IS_OPT_SET_IN(OPT, reg) (reg & (OPT))
#define IS_OPT_SET(OPT) IS_OPT_SET_IN((OPT), prof_session.conf.use_features0)

enum app_type {
	AT_TIZEN	=0x01,
	AT_LAUNCHED	=0x02,
	AT_COMMON	=0x03
};
enum supported_device {
  DEVICE_FLASH,
  DEVICE_CPU
};
static const char *supported_devices_strings[] = {
	"FLASH",
	"CPU"
};
#define array_size(x) (sizeof(x)/sizeof((x)[0]))
enum { supported_devices_count = array_size(supported_devices_strings) };

#define MAX_FILENAME 128

#define MSG_DATA_HDR_LEN 20
struct msg_data_t {
	uint32_t id;
	uint32_t seq_num;
	uint32_t sec;
	uint32_t nsec;
	uint32_t len;
	char payload[0];
};

#define MSG_CMD_HDR_LEN 8
//conf
struct msg_buf_t {
	char *payload;
	char *cur_pos;
	char *end;
	uint32_t len;
};

struct msg_t {
	uint32_t id;
	uint32_t len;
	char payload[0];
};


struct conf_t {
	uint64_t use_features0;
	uint64_t use_features1;
	uint32_t system_trace_period;
	uint32_t data_message_period;
};

typedef uint32_t log_interval_t;

//app, libs, probes
enum app_type_t {
	APP_TYPE_TIZEN = 1,
	APP_TYPE_RUNNING = 2,
	APP_TYPE_COMMON = 3,
};

struct app_info_t {
	uint32_t app_type;
	char *app_id;
	char *exe_path;
};



struct us_func_inst_plane_t {
	uint64_t func_addr;
	char args[0];
};

struct us_lib_inst_t {
	char *bin_path;
};

struct user_space_inst_t {
	uint32_t app_num;
	struct app_list_t *app_inst_list;
	uint32_t lib_num;
	struct lib_list_t *lib_inst_list;
};

//replays
struct replay_event_t {
	uint32_t id;
	struct input_event ev;
};

struct replay_event_seq_t {
	uint32_t enabled;
	struct timeval tv;
	uint32_t event_num;
	struct replay_event_t *events;
};

struct prof_session_t {
	struct conf_t conf;
	struct user_space_inst_t user_space_inst;
	struct replay_event_seq_t replay_event_seq;
	unsigned running_status:1; // to stop properly (1 - it is running, 0 - no)
};

int parseHostMessage(struct msg_t *log, char *msg);
int host_message_handler(struct msg_t *msg);

char *msg_ID_str(enum HostMessageT ID);

// testing
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//data protocol
struct thread_info_t {
	uint32_t pid;
	float load;
};

struct process_info_t {
	uint32_t id;
	float load;
};

struct system_info_t {
	// system_cpu
	float app_cpu_usage;
	float *cpu_frequency;
	float *cpu_load;
	uint32_t count_of_threads;
	struct thread_info_t *thread_load;

	// system_processes
	uint32_t count_of_processes;
	struct process_info_t *process_load;

	// system_memory
	uint32_t virtual_memory;
	uint32_t resident_memory;
	uint32_t shared_memory;
	uint32_t pss_memory;
	uint32_t total_alloc_size;
	uint64_t system_memory_total;
	uint64_t system_memory_used;

	// system_disk
	uint32_t total_used_drive;
	uint32_t disk_reads;
	uint32_t disk_sectors_read;
	uint32_t disk_writes;
	uint32_t disk_sectors_write;

	// system_network
	uint32_t network_send_size;
	uint32_t network_receive_size;

	// system_device
	uint32_t wifi_status;
	uint32_t bt_status;
	uint32_t gps_status;
	uint32_t brightness_status;
	uint32_t camera_status;
	uint32_t sound_status;
	uint32_t audio_status;
	uint32_t vibration_status;
	uint32_t voltage_status;
	uint32_t rssi_status;
	uint32_t video_status;
	uint32_t call_status;
	uint32_t dnet_status;

	// system_energy
	uint32_t energy;
	uint32_t energy_per_device[supported_devices_count];
	uint32_t app_energy_per_device[supported_devices_count];
};

struct recorded_event_t {
	uint32_t id;
	uint32_t type;
	uint32_t code;
	uint32_t value;
};
#define static_assert(cond) \
	char __attribute__((unused)) __static_assert[(cond) ? 1 : -1];

#define pack_int64(to, n) do {						\
		static_assert(sizeof(n) == 8);				\
		*(uint64_t *)to = n;					\
		to += sizeof(uint64_t);					\
	} while (0)

#define pack_int32(to, n) do {						\
		static_assert(sizeof(n) == 4);				\
		*(uint32_t *)to = n;					\
		to += sizeof(uint32_t);					\
	} while (0)

#define pack_time(to, n)						\
	do {								\
		pack_int32(to, n.tv_sec);				\
		pack_int32(to, n.tv_usec);				\
	} while (0)

#define pack_float(to, n)					\
	do {							\
		*(float *)to = n;				\
		to += sizeof(float);				\
	} while (0)

#define pack_str(to, n)				\
	do {					\
		memcpy(to, n, strlen(n) + 1);	\
		to += strlen(n) + 1;		\
	} while (0)

static inline void* pack_str_array(void *buffer, const char **strings,
				   size_t count)
{
	int index;
	for (index = 0; index != count; ++index)
		pack_str(buffer, strings[index]);
	return buffer;
}

struct msg_data_t *pack_system_info(struct system_info_t *sys_info);
int write_to_buf(struct msg_data_t *msg);
void free_msg_data(struct msg_data_t *msg);
void free_msg_payload(struct msg_t *msg);
void free_sys_info(struct system_info_t *sys_info);
int start_replay(void);
void stop_replay(void);

enum ErrorCode stop_all(void);
enum ErrorCode stop_all_no_lock(void);
int stop_all_in_process(void);
void stop_all_done(void);

void reset_msg(struct msg_t *msg);
void reset_replay_event_seq(struct replay_event_seq_t *res);
void reset_system_info(struct system_info_t *sys);
int check_running_status(struct prof_session_t *prof_session);

extern struct prof_session_t prof_session;

//debugs
void print_replay_event(struct replay_event_t *ev, uint32_t num, char *tab);

int sendACKToHost(enum HostMessageT resp, enum ErrorCode err_code,
			char *payload, int payload_size);

int parse_int32(struct msg_buf_t *msg, uint32_t *val);
int parse_int64(struct msg_buf_t *msg, uint64_t *val);
int parse_string(struct msg_buf_t *msg, char **str);
int parse_string_no_alloc(struct msg_buf_t *msg, char *str);
int parse_replay_event_seq(struct msg_buf_t *msg, struct replay_event_seq_t *res);

void init_prof_session(struct prof_session_t *prof_session);
#endif /* _DA_PROTOCOL_ */
