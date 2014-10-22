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
#include <stddef.h>
#include <linux/input.h>

enum HostMessageT {
NMSG_KEEP_ALIVE			=0x0001,
NMSG_START				=0x0002,
NMSG_STOP				=0x0003,
NMSG_CONFIG				=0x0004,
NMSG_BINARY_INFO		=0x0005,
NMSG_GET_TARGET_INFO	=0x0007,
NMSG_SWAP_INST_ADD		=0x0008,
NMSG_SWAP_INST_REMOVE		=0x0009,
NMSG_GET_SCREENSHOT		=0x0010,
NMSG_GET_PROCESS_ADD_INFO	=0x0011,

NMSG_KEEP_ALIVE_ACK			=0x1001,
NMSG_START_ACK				=0x1002,
NMSG_STOP_ACK				=0x1003,
NMSG_CONFIG_ACK				=0x1004,
NMSG_BINARY_INFO_ACK		=0x1005,
NMSG_SWAP_INST_ACK			=0x1006,
NMSG_GET_TARGET_INFO_ACK	=0x1007,
NMSG_SWAP_INST_ADD_ACK		=0x1008,
NMSG_SWAP_INST_REMOVE_ACK	=0x1009,
NMSG_GET_PROCESS_ADD_INFO_ACK	=0x1011,

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
};
#define MSG_MAX_NUM NMSG_SWAP_INST_REMOVE

enum ErrorCode{
	ERR_NO					= 0,	//success
	ERR_LOCKFILE_CREATE_FAILED		= -101,	//lock file create failed
	ERR_ALREADY_RUNNING			= -102,	//already running
	ERR_INITIALIZE_SYSTEM_INFO_FAILED	= -103,	//initialize system info failed
	ERR_HOST_SERVER_SOCKET_CREATE_FAILED	= -104,	//host server socket create failed
	ERR_TARGET_SERVER_SOCKET_CREATE_FAILED	= -105,	//target server socket create failed

	ERR_SIGNAL_MASK_SETTING_FAILED		= -106, //TODO del (old parametr)

	ERR_WRONG_MESSAGE_FORMAT		= -201,	//wrong message format
	ERR_WRONG_MESSAGE_TYPE			= -202,	//wrong message type
	ERR_WRONG_MESSAGE_DATA			= -203,	//wrong message data
	ERR_CANNOT_START_PROFILING		= -204,	//cannot start profiling
	ERR_SERV_SOCK_CREATE			= -900,	//server socket creation failed (written in /tmp/da.port file)
	ERR_SERV_SOCK_BIND			= -901,	//server socket bind failed (written in /tmp/da.port file)
	ERR_SERV_SOCK_LISTEN			= -902,	//server socket listen failed (written in /tmp/da.port file)
	ERR_UNKNOWN				= -999	//unknown error
};

#define FL_SYSTEM_ENERGY_OLD (1<<26)

enum feature_code{
	FL_RESERVED1			= 0x000000000003ULL, // reserved 0011

	FL_FUNCTION_PROFILING		= 0x000000000004ULL, // On/Off the UserSpaceInst
	FL_MEMORY_ALLOC_PROBING		= 0x000000000008ULL, // memory allocation API (glibc)
	FL_FILE_API_PROBING		= 0x000000000010ULL, // file API (glibc, OSP)
	FL_THREAD_API_PROBING		= 0x000000000020ULL, // thread API (glibc, OSP)
	FL_OSP_UI_API_PROBING		= 0x000000000040ULL, // UI API (OSP)
	FL_SCREENSHOT			= 0x000000000080ULL, // Screenshot
	FL_USER_EVENT			= 0x000000000100ULL, // events of Touch, Gesture, Orientation, Key
	FL_RECORDING			= 0x000000000200ULL, // recording the user event
	FL_SYSTCALL_FILE		= 0x000000000400ULL, // File operation syscalls tracing
	FL_SYSTCALL_IPC			= 0x000000000800ULL, // IPC syscall tracing
	FL_SYSTCALL_PROCESS		= 0x000000001000ULL, // Process syscalls tracing
	FL_SYSTCALL_SIGNAL		= 0x000000002000ULL, // Signal syscalls tracing
	FL_SYSTCALL_NETWORK		= 0x000000004000ULL, // Network syscalls tracing
	FL_SYSTCALL_DESC		= 0x000000008000ULL, // Descriptor syscalls tracing
	FL_CONTEXT_SWITCH		= 0x000000010000ULL, // Context switch tracing
	FL_NETWORK_API_PROBING		= 0x000000020000ULL, // network API (glibc, OSP, libsoap, openssl)
	FL_OPENGL_API_PROBING		= 0x000000040000ULL, // openGL API
	FL_FUNCTION_SAMPLING		= 0x000000080000ULL, // Function sampling

	FL_RESERVED2			= 0x00000FF00000ULL, // reserved

	FL_MEMORY_ALLOC_ALWAYS_PROBING	= 0x000010000000ULL, // all (include external) memory allocation API (glibc) always
	FL_FILE_API_ALWAYS_PROBING	= 0x000020000000ULL, // all (include external) file API (glibc, OSP) always
	FL_THREAD_API_ALWAYS_PROBING	= 0x000040000000ULL, // all (include external) thread API (glibc, OSP) always
	FL_OSP_UI_API_ALWAYS_PROBING	= 0x000080000000ULL, // all (include external) UI API (OSP) always
	FL_NETWORK_API_ALWAYS_PROBING	= 0x000100000000ULL, // all (include external) network API (glibc, OSP, libsoap, openssl) always
	FL_OPENGL_API_ALWAYS_PROBING	= 0x000200000000ULL, // all (include external) openGL API always

	FL_RESERVED3			= 0x000c00000000ULL, // reserved

	FL_SYSTEM_CPU			= 0x001000000000ULL, // CPU core load, frequency
	FL_SYSTEM_MEMORY		= 0x002000000000ULL, // System memory used
	FL_SYSTEM_PROCESS		= 0x004000000000ULL, // Info for profilling processes (VSS, PSS, RSS, etc)
	FL_SYSTEM_THREAD_LOAD		= 0x008000000000ULL, // Thread loading for profiling processes
	FL_SYSTEM_PROCESSES_LOAD	= 0x010000000000ULL, // Non instrumented process load
	FL_SYSTEM_DISK			= 0x020000000000ULL, // /proc/diskstats - reads, sectors read, writes, sectors written
	FL_SYSTEM_NETWORK		= 0x040000000000ULL, // network send/recv size
	FL_SYSTEM_DEVICE		= 0x080000000000ULL, //
	FL_SYSTEM_ENERGY		= 0x100000000000ULL, //

	FL_RESERVED4			= 0xe00000000000ULL, // reserved 1110

	FL_ALL_FEATURES			= 0x3FFFFFFFFFFFULL &
					  (~FL_RESERVED1) &
					  (~FL_RESERVED2) &
					  (~FL_RESERVED3) &
					  (~FL_RESERVED4)

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
	DEVICE_CPU,
	DEVICE_LCD
};
static const char *supported_devices_strings[] = {
	"FLASH",
	"CPU",
	"LCD"
};
#define array_size(x) (sizeof(x)/sizeof((x)[0]))
enum { supported_devices_count = array_size(supported_devices_strings) };

#define TARGER_MSG_MAX_LEN			4096
#define HOST_CTL_MSG_MAX_LEN			(10 * 1024 * 1024)

struct _msg_target_t {
	unsigned int type;
	unsigned int length;
	char data[0];
};

struct msg_target_t {
	unsigned int type;
	unsigned int length;
	char data[TARGER_MSG_MAX_LEN];
};

enum { MSG_HEADER_LEN = offsetof(struct msg_target_t, data) };


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
	APP_TYPE_UNKNOWN = 0,
	APP_TYPE_TIZEN = 1,
	APP_TYPE_RUNNING = 2,
	APP_TYPE_COMMON = 3,
	APP_TYPE_WEB = 4
};

struct app_info_t {
	uint32_t app_type;
	char *app_id;
	char *exe_path;
};



struct us_func_inst_plane_t {
	//format
	//name       | type   | len       | info
	//------------------------------------------
	//func_addr  | uint64 | 8         |
	//args       | string | len(args) |end with '\0'
	//ret_type   | char   | 1         |
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

struct other_process_info_t {
	uint32_t id;
	float load;
};

struct inst_process_info_t {
	uint32_t id;
	float load;
	uint64_t virtual_memory;
	uint64_t resident_memory;
	uint64_t shared_memory;
	uint64_t pss_memory;
	uint64_t total_alloc_size;
	uint32_t count_of_threads;

};


/**
 * NOTE:
 *
 * Adding pointer memebers to struct %system_info_t REQUIRES updating code,
 * calculating length of bytevector, suitable for serializing it. See
 * msg_data_payload_length()
 *
 */
struct system_info_t {
	// system_cpu
	float *cpu_frequency;
	float *cpu_load;

	// total thread count to calculate msg size
	uint32_t count_of_threads;

	// system_memory
	uint64_t system_memory_used;

	// system_processes
	uint32_t count_of_inst_processes;
	uint32_t count_of_other_processes;

	//system fd
	uint32_t fd_count;
	uint32_t fd_peration_count;

	// system_disk
	uint32_t total_used_drive;
	uint32_t disk_reads;
	uint32_t disk_bytes_read;
	uint32_t disk_writes;
	uint32_t disk_bytes_write;

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
int check_running_status(const struct prof_session_t *prof_session);

extern struct prof_session_t prof_session;

int send_msg_to_sock(int sock, struct msg_target_t *msg);
int recv_msg_from_sock(int sock, struct msg_target_t *msg);

//debugs
void print_replay_event(struct replay_event_t *ev, uint32_t num, char *tab);

int sendACKToHost(enum HostMessageT resp, enum ErrorCode err_code,
			char *payload, int payload_size);

int parse_int8(struct msg_buf_t *msg, uint8_t *val);
int parse_int32(struct msg_buf_t *msg, uint32_t *val);
int parse_int64(struct msg_buf_t *msg, uint64_t *val);
int parse_string(struct msg_buf_t *msg, char **str);
int parse_string_no_alloc(struct msg_buf_t *msg, char *str);
int parse_replay_event_seq(struct msg_buf_t *msg, struct replay_event_seq_t *res);

void init_prof_session(struct prof_session_t *prof_session);
#endif /* _DA_PROTOCOL_ */
