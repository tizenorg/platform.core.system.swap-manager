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

#define PROTOCOL_VERSION "4.0"


#include "md5.h"
#include "da_msg_ids.h"

enum HostMessageT {

/* msg ids */
	#define X(id, val) id = val,
	MSG_ID_LIST
	#undef X

	NMSG_WRT_LAUNCHER_PORT			=0x8001,
	NMSG_ACK_FLAG				=0x1000,

/* msg ACK */
	#define X(id, val) id ## _ACK = val + NMSG_ACK_FLAG,
	MSG_ID_LIST
	#undef X
};


enum DataMessageT {
	#define X(id, val) id = val,
	DATA_MSG_ID_LIST
	#undef X
};

#define MSG_MAX_NUM NMSG_SWAP_INST_REMOVE

enum ErrorCode {
	ERR_NO					= 0,	/* success */
	ERR_LOCKFILE_CREATE_FAILED		= -101,	/* lock file create failed */
	ERR_ALREADY_RUNNING			= -102,	/* already running */
	ERR_INITIALIZE_SYSTEM_INFO_FAILED	= -103,	/* initialize system info failed */
	ERR_HOST_SERVER_SOCKET_CREATE_FAILED	= -104,	/* host server socket create failed */
	ERR_TARGET_SERVER_SOCKET_CREATE_FAILED	= -105,	/* target server socket create failed */

	ERR_SIGNAL_MASK_SETTING_FAILED		= -106, /* TODO del (old parametr) */

	ERR_WRONG_MESSAGE_FORMAT		= -201,	/* wrong message format */
	ERR_WRONG_MESSAGE_TYPE			= -202,	/* wrong message type */
	ERR_WRONG_MESSAGE_DATA			= -203,	/* wrong message data */
	ERR_CANNOT_START_PROFILING		= -204,	/* cannot start profiling */
	ERR_TO_LONG_MESSAGE			= -205,	/* message is too long to process */
	ERR_TARGET_NOT_FOUND			= -206,	/* some target in message not found like file or some else */
	ERR_NOT_SUPPORTED			= -207,	/* request not supported by security reason */
	ERR_SERV_SOCK_CREATE			= -900,	/* server socket creation failed (written in /tmp/da.port file) */
	ERR_SERV_SOCK_BIND			= -901,	/* server socket bind failed (written in /tmp/da.port file) */
	ERR_SERV_SOCK_LISTEN			= -902,	/* server socket listen failed (written in /tmp/da.port file) */
	ERR_UNKNOWN				= -999	/* unknown error */
};

#define FL_SYSTEM_ENERGY_OLD (1<<26)

enum feature_code{
	FL_RESERVED1			= 0x0000000000003ULL, // reserved 0011

	FL_FUNCTION_PROFILING		= 0x0000000000004ULL, // 0x4 * 0x10^00 On/Off the UserSpaceInst
	FL_MEMORY_ALLOC_PROBING		= 0x0000000000008ULL, // 0x8 * 0x10^00 memory allocation API (glibc)
	FL_FILE_API_PROBING		= 0x0000000000010ULL, // 0x1 * 0x10^01 file API (glibc, OSP)
	FL_THREAD_API_PROBING		= 0x0000000000020ULL, // 0x2 * 0x10^01 thread API (glibc, OSP)
	FL_OSP_UI_API_PROBING		= 0x0000000000040ULL, // 0x4 * 0x10^01 UI API (OSP)
	FL_SCREENSHOT			= 0x0000000000080ULL, // 0x8 * 0x10^01 Screenshot
	FL_USER_EVENT			= 0x0000000000100ULL, // 0x1 * 0x10^02 events of Touch, Gesture, Orientation, Key
	FL_RECORDING			= 0x0000000000200ULL, // 0x2 * 0x10^02 recording the user event
	FL_SYSTCALL_FILE		= 0x0000000000400ULL, // 0x4 * 0x10^02 File operation syscalls tracing
	FL_SYSTCALL_IPC			= 0x0000000000800ULL, // 0x8 * 0x10^02 IPC syscall tracing
	FL_SYSTCALL_PROCESS		= 0x0000000001000ULL, // 0x1 * 0x10^03 Process syscalls tracing
	FL_SYSTCALL_SIGNAL		= 0x0000000002000ULL, // 0x2 * 0x10^03 Signal syscalls tracing
	FL_SYSTCALL_NETWORK		= 0x0000000004000ULL, // 0x4 * 0x10^03 Network syscalls tracing
	FL_SYSTCALL_DESC		= 0x0000000008000ULL, // 0x8 * 0x10^03 Descriptor syscalls tracing
	FL_CONTEXT_SWITCH		= 0x0000000010000ULL, // 0x1 * 0x10^04 Context switch tracing
	FL_NETWORK_API_PROBING		= 0x0000000020000ULL, // 0x2 * 0x10^04 network API (glibc, OSP, libsoap, openssl)
	FL_OPENGL_API_PROBING		= 0x0000000040000ULL, // 0x4 * 0x10^04 openGL API
	FL_FUNCTION_SAMPLING		= 0x0000000080000ULL, // 0x8 * 0x10^04 Function sampling

	FL_RESERVED2			= 0x000000FF00000ULL, // 0x1 * 0x10^00 reserved

	FL_MEMORY_ALLOC_ALWAYS_PROBING	= 0x0000010000000ULL, // 0x1 * 0x10^07 all (include external) memory allocation API (glibc) always
	FL_FILE_API_ALWAYS_PROBING	= 0x0000020000000ULL, // 0x2 * 0x10^07 all (include external) file API (glibc, OSP) always
	FL_THREAD_API_ALWAYS_PROBING	= 0x0000040000000ULL, // 0x4 * 0x10^07 all (include external) thread API (glibc, OSP) always
	FL_OSP_UI_API_ALWAYS_PROBING	= 0x0000080000000ULL, // 0x8 * 0x10^07 all (include external) UI API (OSP) always
	FL_NETWORK_API_ALWAYS_PROBING	= 0x0000100000000ULL, // 0x1 * 0x10^08 all (include external) network API (glibc, OSP, libsoap, openssl) always
	FL_OPENGL_API_ALWAYS_PROBING	= 0x0000200000000ULL, // 0x2 * 0x10^08 all (include external) openGL API always

	FL_RESERVED3			= 0x0000c00000000ULL, // 0x1 * 0x10^00 reserved

	FL_SYSTEM_CPU			= 0x0001000000000ULL, // 0x1 * 0x10^09 CPU core load, frequency
	FL_SYSTEM_MEMORY		= 0x0002000000000ULL, // 0x2 * 0x10^09 System memory used
	FL_SYSTEM_PROCESS		= 0x0004000000000ULL, // 0x4 * 0x10^09 Info for profilling processes (VSS, PSS, RSS, etc)
	FL_SYSTEM_THREAD_LOAD		= 0x0008000000000ULL, // 0x8 * 0x10^09 Thread loading for profiling processes
	FL_SYSTEM_PROCESSES_LOAD	= 0x0010000000000ULL, // 0x1 * 0x10^10 Non instrumented process load
	FL_SYSTEM_DISK			= 0x0020000000000ULL, // 0x2 * 0x10^10 /proc/diskstats - reads, sectors read, writes, sectors written
	FL_SYSTEM_NETWORK		= 0x0040000000000ULL, // 0x4 * 0x10^10 network send/recv size
	FL_SYSTEM_DEVICE		= 0x0080000000000ULL, // 0x8 * 0x10^10
	FL_SYSTEM_ENERGY		= 0x0100000000000ULL, // 0x1 * 0x10^11

	FL_APP_STARTUP			= 0x0200000000000ULL, // 0x2 * 0x10^11 see MSG_APP_SETUP_STAGE in data channel
	FL_WEB_PROFILING		= 0x0400000000000ULL, // 0x4 * 0x10^11 Web profiling
	FL_WEB_STARTUP_PROFILING	= 0x0800000000000ULL, // 0x8 * 0x10^11 Web startup profiling

	FL_SYSTEM_FILE_ACTIVITY		= 0x1000000000000ULL, // 0x1 * 0x10^12 function entry/exit for probe type 04 (File syscall)

	FL_RESERVED4			= 0xe000000000000ULL, // reserved 1110

	FL_ALL_FEATURES			= 0x7FFFFFFFFFFFFULL &
					  (~FL_RESERVED1) &
					  (~FL_RESERVED2) &
					  (~FL_RESERVED3) &
					  (~FL_RESERVED4)

};

enum probe_type {
	SWAP_RETPROBE   = 0, //Common retprobe
	SWAP_FBI_PROBE  = 1, //Function body instrumentation probe
	SWAP_LD_PROBE   = 2, //Preloaded API probe
	SWAP_WEBPROBE   = 3, //Webprobe
};

#define IS_OPT_SET_IN(OPT, reg) (reg & (OPT))
#define IS_OPT_SET(OPT) IS_OPT_SET_IN((OPT), prof_session.conf.use_features0)

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

	struct {
		size_t size;
		char data[8];
	} setup_data;
};



struct us_func_inst_plane_t {
	//format
	//name       | type   | len       | info
	//------------------------------------------
	//func_addr  | uint64 | 8         |
	//probe_type | char   | 1         |
	uint64_t func_addr;
	char probe_type;
	char probe_info[0];
} __attribute__ ((packed));

struct us_lib_inst_t {
	char *bin_path;
};

struct user_space_inst_t {
	uint32_t app_num;
	struct app_list_t *app_inst_list;
	uint32_t lib_num;
	struct lib_list_t *lib_inst_list;
	uint32_t ld_lib_num;
	struct lib_list_t *ld_lib_inst_list;
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
		*(uint64_t *)to = (n);					\
		to += sizeof(uint64_t);					\
	} while (0)

#define pack_int32(to, n) do {						\
		static_assert(sizeof(n) == 4);				\
		*(uint32_t *)to = (n);					\
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
void feature_code_str(uint64_t feature0, uint64_t feature1, char *to,
		      uint32_t buflen);

int sendACKToHost(enum HostMessageT resp, enum ErrorCode err_code,
			char *payload, int payload_size);

int parse_int8(struct msg_buf_t *msg, uint8_t *val);
int parse_int32(struct msg_buf_t *msg, uint32_t *val);
int parse_int64(struct msg_buf_t *msg, uint64_t *val);
int parse_string(struct msg_buf_t *msg, char **str);
int parse_string_no_alloc(struct msg_buf_t *msg, char *str);
int parse_replay_event_seq(struct msg_buf_t *msg, struct replay_event_seq_t *res);

void get_file_md5sum(md5_byte_t digest[16], const char *filename);

void init_prof_session(struct prof_session_t *prof_session);
#endif /* _DA_PROTOCOL_ */
