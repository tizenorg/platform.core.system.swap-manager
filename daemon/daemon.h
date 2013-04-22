/*
*  DA manager
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: 
*
* Jaewon Lim <jaewon81.lim@samsung.com>
* Woojin Jung <woojin2.jung@samsung.com>
* Juyoung Kim <j0.kim@samsung.com>
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
*
*/ 

#include <stdint.h>		// for uint64_t, int64_t
#include <pthread.h>	// for pthread_mutex_t

#ifndef _DAEMON_H_
#define _DAEMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PROTOCOL_VERSION			"2.1"

#define RUN_APP_LOADER

#define MAX_TARGET_COUNT			8
#define DA_MSG_MAX					4096
#define RECV_BUF_MAX				4104	// = sizeof(msg_t)

enum ErrorCode
{
	ERR_LOCKFILE_CREATE_FAILED = -101,
	ERR_ALREADY_RUNNING = -102,
	ERR_INITIALIZE_SYSTEM_INFO_FAILED = -103,
	ERR_HOST_SERVER_SOCKET_CREATE_FAILED = -104,
	ERR_TARGET_SERVER_SOCKET_CREATE_FAILED = -105,
	ERR_WRONG_MESSAGE_FORMAT = -201,
	ERR_WRONG_MESSAGE_TYPE = -202,
	ERR_WRONG_MESSAGE_DATA = -203,
	ERR_CANNOT_START_PROFILING = -204,
	ERR_WRONG_PROTOCOL_VERSION = -205
};

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
	MSG_ERROR = 11
};

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
	MSG_RECORD = 801,
	MSG_REPLAY = 802,
	MSG_OK = 901,
	MSG_NOTOK = 902,
	MSG_VERSION = 999,
	MSG_HOST_END = 999
};

enum DaOptions
{
	OPT_CPUMEM		=	0x00000001,
	OPT_FUNC		=	0x00000002,
	OPT_ALLOC		=	0x00000004,
	OPT_FILE		=	0x00000008,
	OPT_THREAD		=	0x00000010,
	OPT_UI			=	0x00000020,
	OPT_SNAPSHOT	=	0x00000040,
	OPT_EVENT		=	0x00000080,
	OPT_RECORD		=	0x00000100
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

#if DEBUG
	#define LOGI(...)	do{ fprintf(stderr, "[INF]" __VA_ARGS__ ); fflush(stderr); } while(0)
	#define LOGE(...)	do{ fprintf(stderr, "[ERR]" __VA_ARGS__ ); fflush(stderr); } while(0)
	#define LOGW(...)	do{ fprintf(stderr, "[WRN]" __VA_ARGS__ ); fflush(stderr); } while(0)
#else
	#define LOGI(...)	do{} while(0)
	#define LOGE(...)	do{} while(0)
	#define LOGW(...)	do{} while(0)
#endif

#define EVENT_STOP		0x00000001
#define EVENT_PID		0x00000002
#define EVENT_ERROR		0x00000004

typedef struct
{
	unsigned int	type;
	unsigned int	length;
	char			data[DA_MSG_MAX];
} msg_t;

typedef struct
{
	int					control_socket;
	int					data_socket;
	pthread_mutex_t		data_socket_mutex;
} __da_host_info;

typedef struct
{
	uint64_t		starttime;		// written only by recv thread
	int64_t			allocmem;		// written only by recv thread
	int				pid;			// written only by recv thread
	int				socket;			// written only by main thread
	pthread_t		recv_thread;	// written only by main thread
	int				event_fd;		// for thread communication (from recv thread to main thread)
	int				initial_log;	// written only by main thread
} __da_target_info;

typedef struct
{
	FILE*				portfile;
	int					host_server_socket;
	int					target_server_socket;
	int					target_count;
	unsigned int		config_flag;
	pthread_t			sampling_thread;
	__da_host_info		host;
	__da_target_info	target[MAX_TARGET_COUNT];
	char				appPath[128];				// application executable path
} __da_manager;

extern __da_manager manager;

void initialize_log();
int daemonLoop();
int sendDataToHost(msg_t* log);
long long get_total_alloc_size();
void unlink_portfile(void);

int makeRecvThread(int index);
int samplingStart();
int samplingStop();

#ifdef __cplusplus
}
#endif

#endif // _DAEMON_H_
