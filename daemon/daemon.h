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


#ifndef _DAEMON_H_
#define _DAEMON_H_

#include <stdint.h>		// for uint64_t, int64_t
#include <pthread.h>	// for pthread_mutex_t
#include "da_protocol.h"
#include "target.h"

#ifdef __cplusplus
extern "C" {
#endif


#define PROTOCOL_VERSION			"2.1"

#define RUN_APP_LOADER

#define MAX_TARGET_COUNT			8

#define MAX_DEVICE				10
#define MAX_FILENAME			128
/*
enum ErrorCode
{
	ERR_LOCKFILE_CREATE_FAILED = -101,
	ERR_ALREADY_RUNNING = -102,
	ERR_INITIALIZE_SYSTEM_INFO_FAILED = -103,
	ERR_HOST_SERVER_SOCKET_CREATE_FAILED = -104,
	ERR_TARGET_SERVER_SOCKET_CREATE_FAILED = -105,
	ERR_SIGNAL_MASK_SETTING_FAILED = -106,
	ERR_WRONG_MESSAGE_FORMAT = -201,
	ERR_WRONG_MESSAGE_TYPE = -202,
	ERR_WRONG_MESSAGE_DATA = -203,
	ERR_CANNOT_START_PROFILING = -204,
	ERR_WRONG_PROTOCOL_VERSION = -205
};*/

enum TargetMessageType
{
	MSG_DEVICE = 1,
	MSG_TIME = 2,
	MSG_SAMPLE = 3,
	MSG_RESOURCE = 4,
	MSG_LOG = 5,
	MSG_IMAGE = 6,
	MSG_TERMINATE = 7,
	MSG_PID = 8,
	MSG_MSG = 9,
	MSG_ALLOC = 10,
	MSG_ERROR = 11,
	MSG_WARNING = 12
};
#define IS_PROBE_MSG(type) (((type) & 0x0100) == 0x0100)

enum HostMessageType
{
	MSG_HOST_BEGIN = 100,
	MSG_START = 100,
	MSG_STOP = 101,
	MSG_PAUSE = 102,
	MSG_OPTION = 103,
	MSG_ISALIVE = 104,
	MSG_ALIVE = 105,
	MSG_BATT_START = 106,
	MSG_BATT_STOP = 107,
	MSG_CAPTURE_SCREEN = 108,
	MSG_MAPS_INST_LIST = 109,
	MSG_RECORD = 801,
	MSG_REPLAY = 802,
	MSG_OK = 901,
	MSG_NOTOK = 902,
	MSG_VERSION = 999,
	MSG_HOST_END = 999
};

enum DAState
{
	DAS_NONE = 0,
	DAS_START_BEGIN = 1,
	DAS_TARGET_ARM_START = 1,
	DAS_TARGET_X86_START = 2,
	DAS_EMUL_ARM_START = 3,
	DAS_EMUL_X86_START = 4,
	DAS_TARGET_ARM_BATT_START = 5,
	DAS_TARGET_X86_BATT_START = 6,
	DAS_EMUL_ARM_BATT_START = 7,
	DAS_EMUL_X86_BATT_START = 8,
	DAS_START_END = 8,
	DAS_STOP = 9,
	DAS_TERMINATE = 10
};


#ifndef likely
#define likely(x)	__builtin_expect((x), 1)
#define unlikely(x)	__builtin_expect((x), 0)
#endif

#define EVENT_STOP		0x00000001
#define EVENT_PID		0x00000002
#define EVENT_ERROR		0x00000004



typedef struct
{
	int					control_socket;
	int					data_socket;
	pthread_mutex_t		data_socket_mutex;
} __da_host_info;

typedef struct
{
	int brightness;
	int voltage;
	int procmeminfo;
	FILE *video;
	FILE *procstat;
	FILE *networkstat;
	FILE *diskstats;
	FILE *inst_tasks;
} __file_descriptors;

typedef struct
{
	int host_server_socket;
	int target_server_socket;
	int apps_to_run;
	unsigned int config_flag;
	int app_launch_timerfd;
	int connect_timeout_timerfd;
	pthread_t sampling_thread;
	pthread_t replay_thread;
	pthread_t transfer_thread;
	int buf_fd;
	int user_ev_fd;
	int efd;
	__da_host_info host;
	__file_descriptors fd;
	char appPath[128]; // application executable path
} __da_manager;

extern __da_manager manager;


uint64_t get_total_alloc_size(void);
int initialize_log(void);
int daemonLoop(void);
void unlink_portfile(void);

int samplingStart(void);
int samplingStop(void);




// TODO maybe need move to other file
int prepare_profiling(void);
int start_profiling(void);
void stop_profiling(void);
int reconfigure(struct conf_t conf);
int sendACKCodeToHost(enum HostMessageType resp, int msgcode);
void terminate_all(void);

#ifdef __cplusplus
}
#endif

#endif // _DAEMON_H_
