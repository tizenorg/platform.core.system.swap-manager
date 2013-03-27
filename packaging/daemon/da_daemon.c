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

#include <stdio.h>
#include <stdlib.h>		// for realpath
#include <string.h>		// for strtok, strcpy, strncpy
#include <assert.h>		// for assert
#include <limits.h>		// for realpath

#define __USE_GNU
#include <sys/types.h>	// for socket, mkdir, opendir, readdir
#include <sys/socket.h>	// for socket
#include <sys/un.h>		// for sockaddr_un
#include <arpa/inet.h>	// for sockaddr_in, socklen_t
#include <sys/stat.h>	// for chmod, mkdir
#include <sys/time.h>	// for setitimer
#include <signal.h>		// for sigemptyset, sigset_t, sigaddset, ...
#include <unistd.h>		// for unlink
#include <pthread.h>	// for pthread_mutex_t
#include <sys/epoll.h>	// for epoll apis
#include <sys/ipc.h>	// for shmctl
#include <sys/shm.h>	// for shared memory operation
#include <dirent.h>		// for opendir, readdir

#include "utils.h"
#include "sys_stat.h"
#include "da_debug.h"

#define MAX_PATH_LENGTH				256
#define TARGET_CLIENT_COUNT_MAX		8
#define READ_BUF_MAX				4096+8
#define DA_LOG_MAX					4096
#define APP_INSTALL_PATH_MAX		1024
#define UDS_NAME					"/tmp/da.socket"
#define DA_INSTALL_DIR				"/home/developer/sdk_tools/da/"
#define DA_INSTALL_PATH				"/home/developer/sdk_tools/da/da_install_path"
#define DA_BUILD_OPTION				"/home/developer/sdk_tools/da/da_build_option"
#define DA_BASE_ADDRESS				"/home/developer/sdk_tools/da/da_base_address"
#define DA_READELF_PATH				"/home/developer/sdk_tools/da/readelf"
#define SCREENSHOT_DIR				"/tmp/da"
#define HOST_MSG_LENGTH 			3
#define SHAREDMEMKEY				((key_t)463825)

#define TIMER_INTERVAL_SEC			1
#define TIMER_INTERVAL_USEC			0

#define SECOND_INTERVAL
#define MONITORING_INTERVAL			1	// 1 second

#ifdef SECOND_INTERVAL
#define __sleep		sleep
#else
#define __sleep		usleep
#endif

#define RUN_APP_LOADER				1

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
	MSG_APPNAME = 10,
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
	MSG_HOST_END = 107
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

typedef struct
{
	int type;
	int length;
	char data[DA_LOG_MAX];
} log_t;

//TODO :
typedef struct
{
	enum DAState status;
	int serverSockFD;
	int clientSockFD;
} __daHostInfo;

typedef struct
{
	enum DAState status;
	int serverSockFD;
	int connectCount;
	int pidCount;
	int clientSockFD[TARGET_CLIENT_COUNT_MAX];
	int execPID[TARGET_CLIENT_COUNT_MAX];	// exec PID by target
} __daTargetInfo;

typedef struct
{
	long long allocsize;
	long launch_flag;
} __daSharedInfo;

typedef struct
{
	int memid;
	__daSharedInfo* pvalue;
} __daSharedMem;

typedef struct
{
	pthread_t timer_thread;
	pthread_mutex_t sendMutex;
	char appPath[128];					// application executable path
	__daSharedMem sharedmem;
	__daHostInfo iHost;
	__daTargetInfo iTarget;
}__daManager;

__daManager manager =
{
	-1,									// timer_thread handle
	PTHREAD_MUTEX_INITIALIZER,			// pthread_mutex_t sendMutex
	{ 0, },								// char appPath[128]
	{ -1, (void*)-1 },					// __daSharedMem sharedmem
	{ DAS_NONE, -1, -1 },				// __daHostInfo iHost
	{ DAS_NONE, -1, 0, 0, {0, }, {0, }}	// __daTargetInfo iTarget
};

int aul_terminate_pid(int pid);
static void* terminate_thread(void* data);
static void terminate_error(char* errstr, int sendtohost);

#ifdef LOCALTEST
int aul_terminate_pid(int pid)
{
	return kill(pid, SIGTERM);
}
#endif

int _terminate_pid(int pid)
{
	int ret;
	pid_t* pids;
	pthread_t term_thread;

	ret = aul_terminate_pid(pid);

	pids = (pid_t*)malloc(2 * sizeof(pid_t));
	pids[0] = pid;
	pids[1] = -2;

	pthread_create(&term_thread, NULL, terminate_thread, pids);

	return ret;
}

// return 0 if succeed
// return -1 if error occured
static int remove_indir(const char *dirname)
{
	DIR *dir;
	struct dirent *entry;
	char path[MAX_PATH_LENGTH];

	dir = opendir(dirname);
	if(dir == NULL)
	{
		return -1;
	}

	while((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
		{
			snprintf(path, (size_t) MAX_PATH_LENGTH, "%s/%s", dirname, entry->d_name);
			if (entry->d_type != DT_DIR)	// file
			{
				unlink(path);
			}
			else { }	// directory
		}
	}
	closedir(dir);

	return 0;
}

// index is started from 0
int set_launch_flag(int index, int bOn)
{
	int ret = 0;
	// set launch flag before execute application
	if(__builtin_expect(manager.sharedmem.pvalue != (void*)-1, 1))
	{
		if(bOn != 0)
			manager.sharedmem.pvalue->launch_flag |= (1 << index);
		else
			manager.sharedmem.pvalue->launch_flag &= ~(1 << index);
	}
	else 
		ret = -1;

	return ret;
}

long long get_total_alloc_size()
{
	if(__builtin_expect(manager.sharedmem.pvalue != (void*)-1, 1))
	{
		return manager.sharedmem.pvalue->allocsize;
	}
	else
	{
		return 0L;
	}
}

// return 0 for normal case
static int __makeTargetServerSockFD()
{
	struct sockaddr_un serverAddrUn;

	if(manager.iTarget.serverSockFD != -1)
		return -1;	// should be never happend

	// remove pre unix domain socket file
	// remove(UDS_NAME);
	unlink(UDS_NAME);

	if ((manager.iTarget.serverSockFD = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		LOGE("Target server socket creation failed\n");
		return -1;
	}
	bzero(&serverAddrUn, sizeof(serverAddrUn));
	serverAddrUn.sun_family = AF_UNIX;
	sprintf(serverAddrUn.sun_path, "%s", UDS_NAME);

	if (-1 == bind(manager.iTarget.serverSockFD, (struct sockaddr*) &serverAddrUn,
					sizeof(serverAddrUn)))
	{
		LOGE("Target server socket binding failed\n");
		close(manager.iTarget.serverSockFD);
		return -1;
	}

	chmod(serverAddrUn.sun_path, 0777);

	if (-1 == listen(manager.iTarget.serverSockFD, 5))
	{
		LOGE("Target server socket listening failed\n");
		close(manager.iTarget.serverSockFD);
		return -1;
	}

	LOGI("Created TargetSock %d\n", manager.iTarget.serverSockFD);
	return 0;
}

// return 0 for normal case
static int __makeHostServerSockFD()
{
	struct sockaddr_in serverAddrIn;
	int opt = 1;

	if(manager.iHost.serverSockFD != -1)
		return -1;	// should be never happened

	if ((manager.iHost.serverSockFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		LOGE("Host server socket creation failed\n");
		return -1;
	}

	setsockopt(manager.iHost.serverSockFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	memset(&serverAddrIn, 0, sizeof(serverAddrIn));
	serverAddrIn.sin_family = AF_INET;
	serverAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddrIn.sin_port = htons(8001);

	// bind address to server socket
	if (-1 == bind(manager.iHost.serverSockFD, (struct sockaddr*) &serverAddrIn, sizeof(serverAddrIn)))
	{
		LOGE("Host server socket binding failed\n");
		close(manager.iHost.serverSockFD);
		return -1;
	}

	// enter listen state from client
	if (-1 == listen(manager.iHost.serverSockFD, 5))
	{
		LOGE("Host server socket listening failed\n");
		close(manager.iHost.serverSockFD);
		return -1;
	}

	LOGI("Created HostSock %d\n", manager.iHost.serverSockFD);
	return 0;
}

static int __destroySharedMemory()
{
	int ret = 0;
	if(manager.sharedmem.pvalue != (void*)-1)
	{
		ret = shmdt(manager.sharedmem.pvalue);
		if(ret == 0)
			manager.sharedmem.pvalue = (void*)-1;
	}

	if(manager.sharedmem.memid != -1)
	{
		ret = shmctl(manager.sharedmem.memid, IPC_RMID, 0);
		if(ret == 0)
			manager.sharedmem.memid = -1;
	}

	return ret;
}

// return 0 for normal case
// return -1 for error case
static int __createSharedMemory()
{
	manager.sharedmem.memid = shmget(SHAREDMEMKEY, sizeof(__daSharedInfo), IPC_CREAT | 0666);
	if(manager.sharedmem.memid == -1)
	{
		return -1;
	}
	else
	{
		manager.sharedmem.pvalue = (__daSharedInfo*)shmat(manager.sharedmem.memid, NULL, 0);
		if(manager.sharedmem.pvalue == (void*)-1)
		{
			__destroySharedMemory();
			return -1;
		}
		else
		{
			manager.sharedmem.pvalue->allocsize = 0;
			manager.sharedmem.pvalue->launch_flag = 0;
			return 0;
		}
	}
}

/*
static log_t * parsLogData(log_t * log, char* logStr)
{
#if 1
	int i, nPos;

	if (logStr == NULL || log == NULL)
		return NULL;

	//msgType
	for (i = 0, nPos = 0; i < READ_BUF_MAX; i++)
	{
		if (logStr[i] == '|')
		{
			logStr[i] = '\0';
			log->type = atoi(&logStr[nPos]);
			i++;
			break;
		}
		if (logStr[i] == '\0')
			return NULL;
	}
	//length
	for (nPos = i; i < READ_BUF_MAX; i++)
	{
		if (logStr[i] == '|' || logStr[i] == '\0')
		{
			logStr[i] = '\0';
			log->length = atoi(&logStr[nPos]);
			i++;
			break;
		}
	}
	//data
	if ((log->length > 0) && (log->length < DA_LOG_MAX))
	{
		strncpy(log->data, &logStr[i], log->length);
		log->data[log->length] = '\0';
	}

	return log;

#else

	int i;
	char * startPos = logStr;
	char tempBuf[READ_BUF_MAX];

	if(logStr && log)
	{
		log->type = -1;
		log->length = 0;
		log->data[0]='\0';
		//type,length
		for(i=0; i<READ_BUF_MAX; i++)
		{
			if(logStr[i]=='|')
			{
				logStr[i]='\0';
				if(log->type == -1)
				{
					log->type = atoi(startPos);
					startPos = &logStr[i+1];
				}
				else {
					log->length = atoi(startPos);
					startPos = &logStr[i+1];
					break;
				}
			}
		}
		//data
		if((log->length > 0) && (log->length < DA_LOG_MAX) && (startPos))
		{
			strncpy(log->data, startPos, log->length);
			log->data[log->length] = '\0';
		}
		LOGI("log type : %d, length : %d, data : %s\n", log->type, log->length, log->data);
		return log;
	}
	else
	return NULL;
#endif
}
*/

static int get_app_type(void)
{
	int fd;
	char buf[DA_LOG_MAX];

	sprintf(buf, "%s.exe", manager.appPath);
	fd = open(buf, O_RDONLY);
	if(fd != -1)
	{
		close(fd);
		return APP_TYPE_OSP;
	}
	else
	{
		return APP_TYPE_TIZEN;
	}
}

static int get_executable(char* buf, int buflen)
{
	int fd;

	sprintf(buf, "%s.exe", manager.appPath);
	fd = open(buf, O_RDONLY);
	if(fd != -1)
	{
		close(fd);
	}
	else
	{
		strcpy(buf, manager.appPath);
	}
	return 0;
}

static int get_app_install_path(char *strAppInstall, int length)
{
	FILE *fp;
	char buf[DA_LOG_MAX];
	char *p;
	int i;

	if ((fp = fopen(DA_INSTALL_PATH, "r")) == NULL)
	{
		LOGE("Failed to open %s\n", DA_INSTALL_PATH);
		return -1;
	}

	/*ex : <15>   DW_AT_comp_dir    : (indirect string, offset: 0x25f): /home/yt/workspace/templatetest/Debug-Tizen-Emulator	*/
	while (fgets(buf, DA_LOG_MAX, fp) != NULL)
	{
		//name
		p = buf;
		for (i = 0; i < DA_LOG_MAX; i++)
		{
			if (*p == ':')
				break;
			p++;
		}

		if (*p != ':')
			break;
		else
			p++;

		//(...,offset:...)
		for (; i < DA_LOG_MAX; i++)
		{
			if (*p == '(')
			{
				while (*p != ')')
				{
					p++;
				}
			}
			if (*p == ':')
				break;
			p++;
		}

		//find
		if (*p != ':')
			break;
		for (; i < DA_LOG_MAX; i++)
		{
			if (*p == ':' || *p == ' ' || *p == '\t')
				p++;
			else
				break;
		}

		//name
		if (strlen(p) <= length)
		{
			sprintf(strAppInstall, "%s", p);
			for (i = 0; i < strlen(p); i++)
			{
				if (strAppInstall[i] == '\n' || strAppInstall[i] == '\t')
				{
					strAppInstall[i] = '\0';
					break;
				}
			}
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return -1;
}

static int is_app_built_pie(void)
{
	int result;
	FILE *fp;
	char buf[DA_LOG_MAX];

	if((fp = fopen(DA_BUILD_OPTION, "r")) == NULL)
	{
		LOGE("Failed to open %s\n", DA_BUILD_OPTION);
		return -1;
	}

	if(fgets(buf, DA_LOG_MAX, fp) != NULL)
	{
		if(strcmp(buf, "DYN\n") == 0)
			result = 1;
		else if(strcmp(buf, "EXEC\n") == 0)
			result = 0;
		else
			result = -1;
	}
	else
	{
		result = -1;
	}
	fclose(fp);

	return result;
}

static int get_app_base_address(int *baseAddress)
{
	int res;
	FILE *fp;
	char buf[DA_LOG_MAX];

	if((fp = fopen(DA_BASE_ADDRESS, "r")) == NULL)
	{
		LOGE("Failed to open %s\n", DA_BASE_ADDRESS);
		return -1;
	}

	if(fgets(buf, DA_LOG_MAX, fp) != NULL)
	{
		res = sscanf(buf, "%x", baseAddress);
	}
	else
	{
		res = -1;
	}
	fclose(fp);

	return res;
}

static int is_same_app_process(int pid)
{
	int ret = 0;
	FILE *fp;
	char buf[DA_LOG_MAX];
	char cmdPath[PATH_MAX];
	char execPath[PATH_MAX];

	get_executable(execPath, PATH_MAX);
	sprintf(cmdPath, "/proc/%d/cmdline", pid);

	if((fp = fopen(cmdPath, "r")) == NULL)
	{
		return 0;
	}

	if(fgets(buf, DA_LOG_MAX, fp) != NULL)
	{
#if RUN_APP_LOADER
		if(strcmp(buf, manager.appPath) == 0)
#else
		// use execPath instead of manager.appPath 
		if(strcmp(buf, execPath) == 0)
#endif
			ret = 1;
		else
			ret = 0;
	}
	fclose(fp);

	return ret;
}

static int sendMsgStrToHost(char* str, int msgType, int targetNo)
{
	static int initialized = 0;
	int pid;
	int is_pie_build;
	int base_address;
	int app_type;
	char log[DA_LOG_MAX];
	char bufDeviceInfo[DA_LOG_MAX];
	char bufAppInstall[APP_INSTALL_PATH_MAX];

	switch (msgType)
	{
	case MSG_LOG:
	case MSG_RESOURCE:
	case MSG_SAMPLE:
	case MSG_ERROR:
		if(__builtin_expect(initialized == 0, 0))
			return -1;
		sprintf(log, "%d|%s\n", msgType, str);
		break;
	case MSG_PID:
		LOGI("MSG_PID handling : %s\n", str);
		pid = atoi(str);
		if ((manager.iTarget.execPID[targetNo] != -1) && (pid != manager.iTarget.execPID[targetNo]))
		{
			LOGE("Failed to check pid, pid of msg(%d), stored pid(%d)\n", pid, manager.iTarget.execPID[targetNo]);
			return -1;
		}
		LOGI("pid[%d]=%d\n",targetNo,pid);
		manager.iTarget.execPID[targetNo] = pid;
		manager.iTarget.pidCount++;

		{
			char execPath[PATH_MAX];
			char realPath[PATH_MAX];
			char mapsPath[PATH_MAX];
			char appInstallCommand[PATH_MAX];

			get_executable(execPath, PATH_MAX);
			if(realpath(execPath, realPath) == NULL)
			{
				LOGW("Failed to get realpath of app\n");
				strcpy(realPath, execPath);
			}

			sprintf(mapsPath, "/proc/%d/maps", manager.iTarget.execPID[targetNo]);
			sprintf(appInstallCommand,
					"cat %s | grep %s | cut -d\"-\" -f1 > %s", mapsPath,
					realPath, DA_BASE_ADDRESS);
			LOGI("appInstallCommand is %s\n", appInstallCommand);

			do {
				if(access(mapsPath, F_OK) != 0)
					return -1;
				if(is_same_app_process(manager.iTarget.execPID[targetNo]) == 0)
					return -1;

				system(appInstallCommand);
				if(get_app_base_address(&base_address) == 1)
					break;
				sleep(0);
			}
			while(1);
		}
		return 0;
		break;
	case MSG_TIME:
		LOGI("MSG_TIME handling : %s\n", str);
		if (strlen(manager.appPath) > 0)
		{
			is_pie_build = is_app_built_pie();
			get_app_base_address(&base_address);
			app_type = get_app_type();
			bufAppInstall[0] = '\0';
			get_app_install_path(bufAppInstall, APP_INSTALL_PATH_MAX);
			get_device_info(bufDeviceInfo, DA_LOG_MAX);
			if (strlen(bufAppInstall) > 0)
			{
				char buf[PATH_MAX];
				get_executable(buf, PATH_MAX);
				sprintf(log, "%d|%s`,%d`,%s`,%d`,%u`,%d`,%s/%s\n", MSG_DEVICE, bufDeviceInfo,
						manager.iTarget.execPID[targetNo], str, is_pie_build, base_address, app_type,
						bufAppInstall, get_app_name(buf));
			}
			else
				sprintf(log, "%d|%s`,%d`,%s`,%d`,%u`,%d`,\n", MSG_DEVICE, bufDeviceInfo,
						manager.iTarget.execPID[targetNo], str, is_pie_build, base_address, app_type);
			LOGI("MSG_DEVICE msg : %s\n", log);
		}
		else
		{
			sprintf(log, "%d|%d`,%s", MSG_PID, manager.iTarget.execPID[targetNo], str);
			LOGI("MSG_DEVICE msg without appname : %s\n", log);
		}
		initialized = 1;
		break;
	case MSG_TERMINATE:
		LOGI("MSG_TERMINATE handling : connectCount(%d)\n", manager.iTarget.connectCount);
		if(manager.iTarget.connectCount == 1 ){
			sprintf(log, "%d|\n", msgType);
			pthread_mutex_lock(&(manager.sendMutex));
#ifndef LOCALTEST			
			send(manager.iHost.clientSockFD, log, strlen(log), 0);
#else
			LOGI("send to host : %s\n", log);
#endif
			pthread_mutex_unlock(&(manager.sendMutex));
		}
		return 0;
	default:
		sprintf(log, "%d|%s\n", msgType, str);
		break;
	}

	if(manager.iHost.status >= DAS_TARGET_ARM_BATT_START && manager.iHost.status <= DAS_EMUL_X86_BATT_START)
	{
		LOGI("write batt log\n");
		write_batt_log(log);
	}
	else
	{
		if (manager.iHost.clientSockFD != -1)
		{
			pthread_mutex_lock(&(manager.sendMutex));
#ifndef LOCALTEST
			send(manager.iHost.clientSockFD, log, strlen(log), 0);
#else
			LOGI("send to host : %s\n", log);
#endif
			pthread_mutex_unlock(&(manager.sendMutex));
		}
	}
	return 0;
}

static void* timerThread(void* data)
{
	int err, signo;
	char buf[DA_LOG_MAX];
	sigset_t waitsigmask;

	LOGI("Timer thread started\n");

	sigemptyset(&waitsigmask);
	sigaddset(&waitsigmask, SIGALRM);
	sigaddset(&waitsigmask, SIGUSR1);

	while(1)
	{
		err = sigwait(&waitsigmask, &signo);
		if(err != 0)
		{
			LOGE("Failed to sigwait() in timer thread\n");
			continue;
		}
		
		if(signo == SIGALRM)
		{	
			get_resource_info(buf, DA_LOG_MAX, manager.iTarget.execPID, manager.iTarget.pidCount);
			sendMsgStrToHost(buf, MSG_RESOURCE, -1);

			if (manager.iTarget.status < DAS_START_BEGIN || manager.iTarget.status > DAS_START_END )
				break;
		}
		else if(signo == SIGUSR1)
		{
			// end this thread
			break;
		}
		else
		{
			// not happened
			LOGE("This should not be happend in timer thread\n");
		}
	}

	LOGI("Timer thread ended\n");
	return NULL;
}

// return 0 if normal case
// return minus value if critical error
// return plus value if non-critical error
static int timerStart()
{
	sigset_t newsigmask;
	struct itimerval timerval;
//	char buf[DA_LOG_MAX];

	if(manager.timer_thread != -1)	// already started
		return 1;

	sigemptyset(&newsigmask);
	sigaddset(&newsigmask, SIGALRM);
	sigaddset(&newsigmask, SIGUSR1);
	if(pthread_sigmask(SIG_BLOCK, &newsigmask, NULL) != 0)
	{
		LOGE("Failed to signal masking for main thread\n");
		return -1;
	}

	if(pthread_create(&(manager.timer_thread), NULL, timerThread, NULL) < 0)
	{
		LOGE("Failed to create timer thread\n");
		return -1;
	}

	timerval.it_interval.tv_sec = TIMER_INTERVAL_SEC;
	timerval.it_interval.tv_usec = TIMER_INTERVAL_USEC;
	timerval.it_value.tv_sec = TIMER_INTERVAL_SEC;
	timerval.it_value.tv_usec = TIMER_INTERVAL_USEC;
	setitimer(ITIMER_REAL, &timerval, NULL);

	// commected because this resource log send to host before receiving MSG_TIME message from target process
	// send initial value of profiling
//	get_resource_info(buf, DA_LOG_MAX, manager.iTarget.execPID, manager.iTarget.pidCount);
//	sendMsgStrToHost(buf, MSG_RESOURCE, -1);

	return 0;
}

static int timerStop()
{
	if(manager.timer_thread != -1)
	{
//		int status;
//		sigset_t oldsigmask;
		struct itimerval stopval;

//		sigemptyset(&oldsigmask);
//		sigaddset(&oldsigmask, SIGALRM);
//		sigaddset(&oldsigmask, SIGUSR1);

		stopval.it_interval.tv_sec = 0;
		stopval.it_interval.tv_usec = 0;
		stopval.it_value.tv_sec = 0;
		stopval.it_value.tv_usec = 0;

		// stop timer
		setitimer(ITIMER_REAL, &stopval, NULL);

		pthread_kill(manager.timer_thread, SIGUSR1);
//		pthread_join(manager.timer_thread, (void**) &status);

//		if(sigprocmask(SIG_UNBLOCK, &oldsigmask, NULL) < 0)
//		{
//			LOGE("Failed to pthread_sigmask\n");
//		}
		manager.timer_thread = -1;
	}
	__destroySharedMemory();

	return 0;
}

#ifdef USE_BATT_LOG
static void wait_for_starting()
{
	static int chargerfd = -1;
	while (get_file_status(&chargerfd, CHARGERFD))
	{
		LOGI("wait for starting ... \n");
		__sleep(MONITORING_INTERVAL);
	}
}
#endif

static int startProfiling(char* execpath, enum DAState status, long launchflag)
{
	if(__createSharedMemory() < 0)
	{
		LOGE("Failed to create shared memory\n");
		return -1;
	}

	// set launch flag before execute application
	manager.sharedmem.pvalue->launch_flag = launchflag;

	// remove previous screen capture files
	remove_indir(SCREENSHOT_DIR);
	mkdir(SCREENSHOT_DIR, 0777);

	// execute application
	if (exec_app(execpath, get_app_type()))
	{
		manager.iTarget.status = status;
		if(timerStart() < 0)
		{
			return -1;
		}
		LOGI("Timer Started\n");
	}
	else
	{
		return -1;
	}

	return 0;
}

static void* terminate_thread(void* data)
{
	int i;
	FILE* fp;
	size_t readbyte;
	char cmd[MAX_PATH_LENGTH];
	pid_t* pids = (pid_t*)data;

	sleep(1);
	for(i = 0; pids[i] != -2; i++)
	{
		if(pids[i] != -1)
		{
			sprintf(cmd, "ps ax | grep %d | grep -v grep", pids[i]);
			fp = popen(cmd, "r");
			if(fp)
			{
				readbyte = fread(cmd, MAX_PATH_LENGTH - 1, 1, fp);
				if(readbyte > 0)	// process is still alive
				{
					sprintf(cmd, "kill -9 %d", pids[i]);
					system(cmd);
				}
				pclose(fp);
			}
		}
	}

	free(pids);

	return NULL;
}

static void terminate_all_target()
{
	int i;
	pthread_t term_thread;
	pid_t* pids;

	pids = (pid_t*)malloc((manager.iTarget.pidCount + 1) * sizeof(pid_t));

	for (i = 0; i < manager.iTarget.pidCount; i++)
	{
		pids[i] = manager.iTarget.execPID[i];
		if(manager.iTarget.execPID[i] != -1)
		{
			LOGI("TERMINATE  process(%d) by terminate_all_target()\n", manager.iTarget.execPID[i]);
			aul_terminate_pid(manager.iTarget.execPID[i]);
			manager.iTarget.execPID[i] = -1;
		}
	}
	pids[manager.iTarget.pidCount] = -2;

	pthread_create(&term_thread, NULL, terminate_thread, pids);

	manager.iTarget.pidCount = 0;
}

static void terminate_error(char* errstr, int sendtohost)
{
	LOGE("%s, status(%d)\n", errstr, manager.iHost.status);
	manager.iHost.status = DAS_STOP;

	terminate_all_target();

	timerStop();

	if(sendtohost)
		sendMsgStrToHost(errstr, MSG_ERROR, -1);
}


// return 0 if normal case
// return plus value if non critical error occur
// return minus value if critical error occur
static int hostMessageHandler(log_t * log)
{
	int ret = 0;
	long flag = 0;
	char *barloc, *tmploc;
	char execPath[PATH_MAX];

	if (log == NULL)
		return 1;

	switch (log->type)
	{
	case MSG_START:
		LOGI("MSG_START handling : %s\n", log->data);
		if (manager.iHost.status >= DAS_START_BEGIN	&& manager.iHost.status <= DAS_START_END)
		{
			LOGE("MSG_START status check : %d\n", manager.iHost.status);
			return 1;		// already started, then ignore this MSG_START message
		}

		if(log->length == 0)
			return -1;		// wrong message format

		// parsing for host start status
		tmploc  = log->data;
		barloc = strchr(tmploc, '|');
		if(barloc != NULL)
		{
			manager.iHost.status = 0;
			while(tmploc < barloc)
			{
				manager.iHost.status = (manager.iHost.status * 2) + (*tmploc - '0');
				tmploc++;
			}
			manager.iHost.status += 1;

			if(manager.iHost.status < DAS_START_BEGIN || manager.iHost.status > DAS_START_END)
			{
				manager.iHost.status = DAS_EMUL_X86_START;
				ret = 1;
			}
		}
		else
		{
			return -1;	// wrong message format
		}

		// parsing for target launch option flag
		tmploc = barloc + 1;
		barloc = strchr(tmploc, '|');
		if(barloc != NULL)
		{
			while(tmploc < barloc)
			{
				flag = (flag * 2) + (*tmploc - '0');
				tmploc++;
			}
		}
		else
		{
			return -1;	// wrong message format
		}

		// parsing for application package name
		tmploc = barloc + 1;
		strcpy(manager.appPath, tmploc);

#ifdef USE_BATT_LOG
		if(manager.iHost.status == DAS_EMUL_ARM_BATT_START ||
				manager.iHost.status == DAS_EMUL_X86_BATT_START)
			wait_for_starting();
#endif

		get_executable(execPath, PATH_MAX); // get exact app executable file name
#if RUN_APP_LOADER
		kill_app(manager.appPath);
#else
		// use execPath instead of manager.appPath
		kill_app(execPath);
#endif
		sleep(0);
	
		LOGI("executable app path %s\n", manager.appPath);

		{
			char appInstallCommand[PATH_MAX];

			//save app install path
			mkdir(DA_INSTALL_DIR, 0775);
			sprintf(appInstallCommand,
					"%s -Wwi %s | grep DW_AT_comp_dir > %s", DA_READELF_PATH,
					execPath, DA_INSTALL_PATH);
			LOGI("appInstallCommand %s\n", appInstallCommand);
			system(appInstallCommand);

			sprintf(appInstallCommand,
					"%s -h %s | grep Type | cut -d\" \" -f33 > %s", DA_READELF_PATH,
					execPath, DA_BUILD_OPTION);
			LOGI("appInstallCommand %s\n", appInstallCommand);
			system(appInstallCommand);

#if RUN_APP_LOADER
			if(startProfiling(manager.appPath, manager.iHost.status, flag) < 0)
#else
			// use execPath instead of manager.appPath
			if(startProfiling(execPath, manager.iHost.status, flag) < 0)
#endif
			{
				terminate_error("Cannot start profiling", 1);
				return -1;
			}
		}
		break;
	case MSG_STOP:
		LOGI("MSG_STOP handling\n");
		if (manager.iHost.status < DAS_START_BEGIN || manager.iHost.status > DAS_START_END)
		{
			// already stopped, not possible, ignore MSG_STOP message
			LOGE("MSG_STOP status check : %d\n", manager.iHost.status);
			return 1;
		}

		manager.iHost.status = DAS_STOP;
		terminate_all_target();
		timerStop();
		break;
	case MSG_OPTION:
		if(log->length > 0)
		{
			if(log->data[0] == '0')
			{
				set_launch_flag(1, 0);
				LOGI("Snapshot disabled\n");
			}
			else if(log->data[0] == '1')
			{
				set_launch_flag(1, 1);
				LOGI("Snapshot enabled\n");
			}
			else
			{
				LOGI("Wrong option message from host\n");
			}
		}
		break;
	case MSG_ISALIVE:
		sendMsgStrToHost(NULL, MSG_ALIVE, 0);
		break;

#ifdef USE_BATT_LOG
	case MSG_BATT_START:
		{
			manager.iHost.status = DAS_BATT_START;
			sprintf(manager.appPath, "%s", log->data);
			create_open_batt_log(get_app_name(manager.appPath));
			wait_for_starting();
			if(startProfiling(manager.appPath, DAS_BATT_START, flag) < 0)
			{
				terminate_error("Cannot start profiling", 1);
				return -1;
			}
//			batt_start(manager.appPath);
		}
		break;
	case MSG_BATT_STOP:
		manager.iHost.status = DAS_BATT_STOP;
		timerStop();
//		batt_stop();
		break;
#endif

	default:
		LOGW("Unknown msg\n");
	}

	return ret;
}

static int parseHostMessage(log_t* log, char* msg)
{
	int i;
	int bfind = 0;

	if(log == NULL || msg == NULL)
		return 0;

	// Host message looks like this
	// MSG_TYPE|MSG_LENGTH|MSG_STRING
	// MSG_TYPE is always 3 digits number
	if(msg[3] == '|')
	{
		msg[3] = '\0';
		log->type = atoi(msg);

		if(log->type < MSG_HOST_BEGIN || log->type > MSG_HOST_END)
			return 0;

		msg = msg + 4;
		for(i = 0; msg[i] != '\0'; i++)
		{
			if(msg[i] == '|')
			{
				bfind = 1;
				msg[i] = '\0';
				break;
			}
		}

		if(bfind != 0)
		{
			int msglen;
			log->length = atoi(msg);
			msg = msg + i + 1;
			msglen = strlen(msg);

			if(log->length == 0)
			{
				log->data[0] = '\0';
			}
			else
			{
				if(msglen == log->length)
					strcpy(log->data, msg);
				else if(msglen > log->length)	// parsing error but deal as nonerror
					strncpy(log->data, msg, log->length);
				else
					return 0;	// parsing error

				log->data[log->length] = '\0';
			}
		}
		else
		{
			return 0;	// parsing error
		}

		return 1;	// parsing success
	}
	else
	{
		return 0;	// parsing error
	}
}

#define EPOLL_SIZE 10
#define MAX_CONNECT_SIZE 12
#define MAX_INAROW_TARGET_MSG 10

// return 0 for normal case
static int work()
{
	struct sockaddr_un clientAddrUn; //target
	struct sockaddr_in clientAddrIn; //host
	int i, k, find;
	ssize_t recvLen;
	char recvBuf[READ_BUF_MAX];
	log_t log;

	struct epoll_event ev, *events;
	int efd;	// epoll fd
	int numevent;	// number of occured events

	// initialize epoll event pool
	events = (struct epoll_event*) malloc(sizeof(struct epoll_event) * EPOLL_SIZE);
	if(events == NULL)
	{
		LOGE("Out of memory when allocate epoll event pool\n");
		return -1;
	}
	if((efd = epoll_create(MAX_CONNECT_SIZE)) < 0)
	{
		LOGE("epoll creation error\n");
		free(events);
		return -1;
	}

	// add server sockets to epoll event pool
	ev.events = EPOLLIN;
	ev.data.fd = manager.iHost.serverSockFD;
	if(epoll_ctl(efd, EPOLL_CTL_ADD, manager.iHost.serverSockFD, &ev) < 0)
	{
		LOGE("Host server socket epoll_ctl error\n");
		free(events);
		close(efd);
		return -1;
	}
	ev.events = EPOLLIN;
	ev.data.fd = manager.iTarget.serverSockFD;
	if(epoll_ctl(efd, EPOLL_CTL_ADD, manager.iTarget.serverSockFD, &ev) < 0)
	{
		LOGE("Target server socket epoll_ctl error\n");
		free(events);
		close(efd);
		return -1;
	}

	// handler loop
	while (1)
	{
		numevent = epoll_wait(efd, events, EPOLL_SIZE, -1);
		if(numevent <= 0)
		{
			LOGE("Failed to epoll_wait : num of event(%d), errno(%d)\n", numevent, errno);
			continue;
		}

		for(i = 0; i < numevent; i++)
		{
			find = 0;
			for(k = 0; k < manager.iTarget.connectCount; k++)
			{
				if(events[i].data.fd == manager.iTarget.clientSockFD[k])
				{
					// read from target process
					recvLen = recv(manager.iTarget.clientSockFD[k], &log,
							sizeof(log.type) + sizeof(log.length), 0);

					// send to host
					if (recvLen > 0)
					{
						int count = 0;
						do {
							if (log.length > 0)
								recv(manager.iTarget.clientSockFD[k], log.data, log.length, 0);
							log.data[log.length] = '\0';
							if(log.type == 5)	// MSG_LOG
							{
								switch(log.data[0])
								{
									case '2':	// UI control creation log
									case '3':	// UI event log
									case '6':	// UI lifecycle log
									case '7':	// screnshot log
									case '8':	// scene transition log
										LOGI("%dclass|%s\n", log.data[0] - '0', log.data);
										break;
									default:
										break;
								}
							}
							else if(log.type == 6)	// MSG_IMAGE
							{
								LOGI("MSG_IMAGE received\n");
							}
							else 	// not MSG_LOG and not MSG_IMAGE
							{
								LOGI("Extra MSG TYPE (%d|%d|%s)\n", log.type, log.length, log.data);
							}
							sendMsgStrToHost(log.data, log.type, k);
							if(count++ > MAX_INAROW_TARGET_MSG)
								break;
						} while ((recvLen = recv(manager.iTarget.clientSockFD[k],
									&log, sizeof(log.type) + sizeof(log.length), MSG_DONTWAIT)) > 0);
					}

					// close request from target client socket
					if(recvLen == 0)
					{
						LOGI("target close = %d(total %d)\n", manager.iTarget.clientSockFD[k], manager.iTarget.connectCount - 1);
						if(epoll_ctl(efd, EPOLL_CTL_DEL, manager.iTarget.clientSockFD[k], NULL) < 0)
						{
							LOGE("Failed to epoll_ctl delete fd from event poll\n");
						}
						close(manager.iTarget.clientSockFD[k]);
						LOGI("Terminate %dth pid(%d)\n", k, manager.iTarget.execPID[k]);
						// consume that target process is already in terminating phase
//						_terminate_pid(manager.iTarget.execPID[k]);
						manager.iTarget.connectCount--;
						if(manager.iTarget.execPID[k] != -1)
							manager.iTarget.pidCount--;
//						assert(manager.iTarget.connectCount == manager.iTarget.pidCount);
						if (manager.iTarget.connectCount == 0)	// all target client are closed
						{
							manager.iTarget.status = DAS_TERMINATE;

							LOGI("Exit daemon process\n");
							manager.iTarget.clientSockFD[k] = -1;
							manager.iTarget.execPID[k] = -1;
							timerStop();
							free(events);
							close(efd);
							return 0;
						}
						else
						{
							manager.iTarget.clientSockFD[k] = manager.iTarget.clientSockFD[manager.iTarget.connectCount];
							manager.iTarget.clientSockFD[manager.iTarget.connectCount] = -1;
							manager.iTarget.execPID[k] = manager.iTarget.execPID[manager.iTarget.pidCount];
							manager.iTarget.execPID[manager.iTarget.pidCount] = -1;
						}
					}

					find = 1;
					break;
				}
			}
			if(find == 0)	// this event is not from target client socket
			{
				// connect request from target
				if(events[i].data.fd == manager.iTarget.serverSockFD)
				{
					socklen_t addrlen;
					addrlen = sizeof(clientAddrUn);
					manager.iTarget.clientSockFD[manager.iTarget.connectCount] =
							accept(manager.iTarget.serverSockFD, (struct sockaddr *) &clientAddrUn, &addrlen);

					if(manager.iTarget.clientSockFD[manager.iTarget.connectCount] >= 0)	// accept succeed
					{
						ev.events = EPOLLIN;
						ev.data.fd = manager.iTarget.clientSockFD[manager.iTarget.connectCount];
						if(epoll_ctl(efd, EPOLL_CTL_ADD, manager.iTarget.clientSockFD[manager.iTarget.connectCount], &ev) < 0)
						{
							// consider as accept fail
							LOGE("Target client epoll_ctl error\n");
							manager.iTarget.clientSockFD[manager.iTarget.connectCount] = -1;
						}
						else
						{
							LOGI("target connect = %d(total%d)\n",
									manager.iTarget.clientSockFD[manager.iTarget.connectCount], manager.iTarget.connectCount + 1);
							manager.iTarget.connectCount++;
						}
					}
					else	// accept error
					{
						LOGE("Failed to accept from target server socket\n");
					}
				}
				// connect request from host
				else if(events[i].data.fd == manager.iHost.serverSockFD)
				{
					socklen_t addrlen;
					addrlen = sizeof(clientAddrIn);
					if(manager.iHost.clientSockFD != -1)
					{
						if(epoll_ctl(efd, EPOLL_CTL_DEL, manager.iHost.clientSockFD, NULL) < 0)
						{
							LOGE("Failed to delete host client socket from epoll ctrl queue\n");
						}
						close(manager.iHost.clientSockFD);
						LOGI("Host client socket is closed, fd(%d)\n", manager.iHost.clientSockFD);
					}
					manager.iHost.clientSockFD = accept(manager.iHost.serverSockFD, (struct sockaddr *) &clientAddrIn, &addrlen);

					if(manager.iHost.clientSockFD >= 0)		// accept succeed
					{
						ev.events = EPOLLIN;
						ev.data.fd = manager.iHost.clientSockFD;
						if(epoll_ctl(efd, EPOLL_CTL_ADD, manager.iHost.clientSockFD, &ev) < 0)
						{
							// consider as accept fail
							close(manager.iHost.clientSockFD);
							manager.iHost.clientSockFD = -1;
							terminate_error("Host client epoll_ctl error", 1);
						}
						else
						{
							LOGI("host connect = %d\n", manager.iHost.clientSockFD);
						}
					}
					else	// accept error
					{
						LOGE("Failed to accept from host server socket\n");
					}
				}
				// message from host
				else if(events[i].data.fd == manager.iHost.clientSockFD)
				{
					// host log format xxx|length|str
					LOGI("Host client socket selected(Message from host)\n");
					recvLen = recv(manager.iHost.clientSockFD, recvBuf, READ_BUF_MAX, 0);

					if (recvLen > 0)
					{
						recvBuf[recvLen] = '\0';
						LOGI("host sent this msg len(%d) str(%s)\n", recvLen, recvBuf);
						if(parseHostMessage(&log, recvBuf) == 0)
						{
							// error to parse host message
							sendMsgStrToHost("host log message is unrecognizable", MSG_ERROR, -1);
							continue;
						}

						//host msg command handling
						if(hostMessageHandler(&log) < 0)
						{
							terminate_error("Host message handling error", 1);
							free(events);
							close(efd);
							return 0;
						}
					}
					else	// close request from HOST
					{
						// work loop quit
						if(manager.iHost.status >= DAS_START_BEGIN && manager.iHost.status <= DAS_START_END)
						{
							// host client socket end unexpectly
							terminate_error("Host client socket closed unexpectly", 0);
						}
						LOGI("host close = %d\n", manager.iHost.clientSockFD);
						free(events);
						close(efd);
						return 0;
					}
				}
				else
				{
					// never happened
					LOGW("Unknown socket fd\n");
				}
			}
		}
	}

	free(events);
	close(efd);
	return 0;
}

// return 0 for normal case
static int initialize_manager()
{
	int i;
	// make server socket
	if(__makeTargetServerSockFD() != 0)
		return -1;
	if(__makeHostServerSockFD() != 0)
		return -1;

	// initialize target client sockets
	for (i = 0; i < TARGET_CLIENT_COUNT_MAX; i++)
	{
		manager.iTarget.clientSockFD[i] = -1;
		manager.iTarget.execPID[i] = -1;
	}

	// initialize sendMutex
	pthread_mutex_init(&(manager.sendMutex), NULL);

	if(initialize_system_info() < 0)
		return -1;

	return 0;
}

static int finalize_manager()
{
	int i;

	finalize_system_info();

	terminate_all_target();

	LOGI("Finalize daemon\n");

	// finalize target client sockets
	for (i = 0; i < TARGET_CLIENT_COUNT_MAX; i++)
	{
		if(manager.iTarget.clientSockFD[i] != -1)
			close(manager.iTarget.clientSockFD[i]);
	}

	// close host client socket
	if(manager.iHost.clientSockFD != -1)
		close(manager.iHost.clientSockFD);

	// close server socket
	if(manager.iHost.serverSockFD != -1)
		close(manager.iHost.serverSockFD);
	if(manager.iTarget.serverSockFD != -1)
		close(manager.iTarget.serverSockFD);

	return 0;
}

static void __attribute((destructor)) exit_func()
{
	__destroySharedMemory();
}

int main()
{
#if DEBUG
	write_log();
#endif

	//for terminal exit
	signal(SIGHUP, SIG_IGN);
	chdir("/");

	//new session reader
	setsid();

	// initialize manager
	if(initialize_manager() == 0)
	{
		//daemon work
		work();

		finalize_manager();
		return 0;
	}
	else
		return 1;
}
