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
#include <stdlib.h>			// for realpath
#include <string.h>			// for strtok, strcpy, strncpy
#include <limits.h>			// for realpath

#include <errno.h>			// for errno
#include <sys/types.h>		// for accept, mkdir, opendir, readdir
#include <sys/socket.h>		// for accept
#include <sys/stat.h>		// for mkdir
#include <sys/eventfd.h>	// for eventfd
#include <sys/epoll.h>		// for epoll apis
#include <unistd.h>			// for access, sleep
#include <attr/xattr.h>		// for fsetxattr
#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>

#include "daemon.h"
#include "sys_stat.h"
#include "utils.h"

#define DA_WORK_DIR				"/home/developer/sdk_tools/da/"
#define DA_READELF_PATH			"/home/developer/sdk_tools/da/readelf"
#define SCREENSHOT_DIR			"/tmp/da"

#define EPOLL_SIZE				10
#define MAX_CONNECT_SIZE		12

#define INPUT_ID_TOUCH			0
#define INPUT_ID_KEY			1	
#define STR_TOUCH				"TOUCH"
#define STR_KEY					"KEY"
#define INPUT_ID_STR_KEY		"ID_INPUT_KEY=1"
#define INPUT_ID_STR_TOUCH		"ID_INPUT_TOUCHSCREEN=1"
#define INPUT_ID_STR_KEYBOARD	"ID_INPUT_KEYBOARD=1"
#define INPUT_ID_STR_TABLET		"ID_INPUT_TABLET=1"

#define MAX_DEVICE				10
#define MAX_FILENAME			128
#define BUF_SIZE				1024
#define ARRAY_END				(-11)

typedef struct _input_dev
{
	int fd;
	char fileName[MAX_FILENAME];
} input_dev;

input_dev g_key_dev[MAX_DEVICE];
input_dev g_touch_dev[MAX_DEVICE];

// return bytes size of readed data
// return 0 if no data readed or error occurred
static int _file_read(FILE* fp, char *buffer, int size)
{
	int ret = 0;

	if(fp != NULL && size > 0)
	{
		ret = fread((void*)buffer, sizeof(char), size, fp);
	}
	else
	{
		// fp is null
		if(size > 0)
			buffer[0] = '\0';

		ret = 0;	// error case
	}

	return ret;
}

// get input id of given input device
static int get_input_id(char* inputname)
{
	static int query_cmd_type = 0;	// 1 if /lib/udev/input_id, 2 if udevadm
	FILE* cmd_fp = NULL;
	char buffer[BUF_SIZE];
	char command[MAX_FILENAME];
	int ret = -1;

	// determine input_id query command
	if(unlikely(query_cmd_type == 0))
	{
		if(access("/lib/udev/input_id", F_OK) == 0)		// there is /lib/udev/input_id
		{
			query_cmd_type = 1;
		}
		else	// there is not /lib/udev/input_id
		{
			query_cmd_type = 2;
		}
	}

	// make command string
	if(query_cmd_type == 1)
	{
		sprintf(command, "/lib/udev/input_id /class/input/%s", inputname);
	}
	else
	{
		sprintf(command, "udevadm info --name=input/%s --query=property", inputname);
	}

	// run command
	cmd_fp = popen(command, "r");
	_file_read(cmd_fp, buffer, BUF_SIZE);

	// determine input id
	if(strstr(buffer, INPUT_ID_STR_KEY))			// key
	{
		ret = INPUT_ID_KEY;
	}
	else if(strstr(buffer, INPUT_ID_STR_TOUCH))		// touch
	{
		ret = INPUT_ID_TOUCH;
	}
	else if(strstr(buffer, INPUT_ID_STR_KEYBOARD))	// keyboard
	{
		ret = INPUT_ID_KEY;
	}
	else if(strstr(buffer, INPUT_ID_STR_TABLET))	// touch (emulator)
	{
		ret = INPUT_ID_TOUCH;
	}

	if(cmd_fp != NULL)
		pclose(cmd_fp);
	return ret;
}

// get filename and fd of given input type devices
static void _get_fds(input_dev *dev, int input_id)
{
	DIR *dp;
	struct dirent *d;
	int count = 0;

	dp = opendir("/sys/class/input");

	if(dp != NULL)
	{
		while((d = readdir(dp)) != NULL)
		{
			if(!strncmp(d->d_name, "event", 5))	// start with "event"
			{
				// event file
				if(input_id == get_input_id(d->d_name))
				{
					sprintf(dev[count].fileName, "/dev/input/%s", d->d_name);
					dev[count].fd = open(dev[count].fileName, O_RDWR);
					count++;
				}
			}
		}

		closedir(dp);
	}
	dev[count].fd = ARRAY_END;	// end of input_dev array
}

static void _device_write(input_dev *dev, struct input_event* in_ev)
{
	int i;
	for(i = 0; dev[i].fd != ARRAY_END; i++)
	{
		if(dev[i].fd >= 0)
			write(dev[i].fd, in_ev, sizeof(struct input_event));
	}
}

long long get_total_alloc_size()
{
	int i;
	long long allocsize = 0;

	for(i = 0; i < MAX_TARGET_COUNT; i++)
	{
		if(manager.target[i].socket != -1 && manager.target[i].allocmem > 0)
			allocsize += manager.target[i].allocmem;
	}
	return allocsize;
}

static int getEmptyTargetSlot()
{
	int i;
	for(i = 0; i < MAX_TARGET_COUNT; i++)
	{
		if(manager.target[i].socket == -1)
			break;
	}

	return i;
}

static void setEmptyTargetSlot(int index)
{
	if(index >= 0 && index < MAX_TARGET_COUNT)
	{
		manager.target[index].pid = -1;
		manager.target[index].recv_thread = -1;
		manager.target[index].allocmem = 0;
		manager.target[index].starttime	= 0;
		manager.target[index].initial_log = 0;
		if(manager.target[index].event_fd != -1)
			close(manager.target[index].event_fd);
		manager.target[index].event_fd = -1;
		if(manager.target[index].socket != -1)
			close(manager.target[index].socket);
		manager.target[index].socket = -1;
	}
}

// ======================================================================================
// send functions to host
// ======================================================================================

int sendDataToHost(msg_t* log)
{
	if (manager.host.data_socket != -1)
	{
		char logstr[DA_MSG_MAX];
		int loglen;

		if(log->length != 0)
			loglen = sprintf(logstr, "%d|%d|%s\n", log->type, log->length + 1, log->data);
		else
			loglen = sprintf(logstr, "%d|%d|\n", log->type, log->length + 1);

//		loglen = sprintf(logstr, "%d|%s\n", log->type, log->data);

		pthread_mutex_lock(&(manager.host.data_socket_mutex));
		send(manager.host.data_socket, logstr, loglen, MSG_NOSIGNAL);
		pthread_mutex_unlock(&(manager.host.data_socket_mutex));
		return 0;
	}
	else
		return 1;
}

// msgstr can be NULL
static int sendACKStrToHost(enum HostMessageType resp, char* msgstr)
{
	if (manager.host.control_socket != -1)
	{
		char logstr[DA_MSG_MAX];
		int loglen;

		if(msgstr != NULL)
			loglen = sprintf(logstr, "%d|%d|%s", (int)resp, strlen(msgstr), msgstr);
		else
			loglen = sprintf(logstr, "%d|0|", (int)resp);

		send(manager.host.control_socket, logstr, loglen, MSG_NOSIGNAL);
		return 0;
	}
	else
		return 1;
}

static int sendACKCodeToHost(enum HostMessageType resp, int msgcode)
{
	if (manager.host.control_socket != -1)
	{
		char codestr[16];
		char logstr[DA_MSG_MAX];
		int loglen, codelen;

		codelen = sprintf(codestr, "%d", msgcode);
		loglen = sprintf(logstr, "%d|%d|%s", (int)resp, codelen, codestr);

		send(manager.host.control_socket, logstr, loglen, MSG_NOSIGNAL);
		return 0;
	}
	else
		return 1;
}

// ========================================================================================
// start and terminate control functions
// ========================================================================================

static int startProfiling(long launchflag)
{
	char execPath[PATH_MAX];

	// remove previous screen capture files
	remove_indir(SCREENSHOT_DIR);
	mkdir(SCREENSHOT_DIR, 0777);

	manager.config_flag = launchflag;

#ifdef RUN_APP_LOADER
	strcpy(execPath, manager.appPath);
#else
	get_executable(manager.appPath, execPath, PATH_MAX);
#endif
	if(exec_app(execPath, get_app_type(manager.appPath)))
	{
		if(samplingStart() < 0)
		{
			return -1;
		}
		LOGI("Timer Started\n");
	}
	else
		return -1;

	return 0;
}

// terminate single target
// just send stop message to target process
static void terminate_target(int index)
{
	ssize_t sendlen;
	msg_t sendlog;
	sendlog.type = MSG_STOP;
	sendlog.length = 0;

	if(manager.target[index].socket != -1)
	{
		// result of sending to disconnected socket is not expected
		sendlen = send(manager.target[index].socket, &sendlog, sizeof(sendlog.type) + sizeof(sendlog.length), MSG_NOSIGNAL);
		if(sendlen != -1)
		{
			LOGI("TERMINATE send exit msg (socket %d) by terminate_target()\n", manager.target[index].socket);
		}
	}
}

// just send stop message to all target process
static void terminate_all_target()
{
	int i;
	ssize_t sendlen;
	msg_t sendlog;

	sendlog.type = MSG_STOP;
	sendlog.length = 0;

	for (i = 0; i < MAX_TARGET_COUNT; i++)
	{
		if(manager.target[i].socket != -1)
		{
			sendlen = send(manager.target[i].socket, &sendlog, sizeof(sendlog.type) + sizeof(sendlog.length), MSG_NOSIGNAL);
			if(sendlen != -1)
			{
				LOGI("TERMINATE send exit msg (socket %d) by terminate_all_target()\n", manager.target[i].socket);
			}
		}
	}
}

// terminate all target and wait for threads
static void terminate_all()
{
	int i;
	terminate_all_target();
	samplingStop();

	// wait for all other thread exit
	for(i = 0; i < MAX_TARGET_COUNT; i++)
	{
		if(manager.target[i].recv_thread != -1)
		{
			pthread_join(manager.target[i].recv_thread, NULL);
		}
	}
}

// terminate all profiling by critical error
static void terminate_error(char* errstr, int sendtohost)
{
	msg_t log;

	LOGE("TERMINATE ERROR: %s\n", errstr);
	if(sendtohost)
	{
		log.type = MSG_ERROR;
		log.length = sprintf(log.data, "%s", errstr);
		sendDataToHost(&log);
	}

	terminate_all();
}

// ===========================================================================================
// message parsing and handling functions
// ===========================================================================================

static int parseDeviceMessage(msg_t* log)
{
	char eventType[MAX_FILENAME];
	struct input_event in_ev;
	int i, index;

	if(log == NULL)
		return -1;

	eventType[0] = '\0';
	in_ev.type = 0;
	in_ev.code = 0;
	in_ev.value = 0;

	index = 0;
	for(i = 0; i < log->length; i++)
	{
		if(log->data[i] == '\n')
			break;

		if(log->data[i] == '`')	// meet separate
		{
			i++;
			index++;
			continue;
		}

		if(index == 0)		// parse eventType
		{
			eventType[i] = log->data[i];
			eventType[i+1] = '\0';
		}
		else if(index == 1)	// parse in_ev.type
		{
			in_ev.type = in_ev.type * 10 + (log->data[i] - '0');
		}
		else if(index == 2)	// parse in_ev.code
		{
			in_ev.code = in_ev.code * 10 + (log->data[i] - '0');
		}
		else if(index == 3)	// parse in_ev.value
		{
			in_ev.value = in_ev.value * 10 + (log->data[i] - '0');
		}
	}

	if(index != 3)
		return -1;	// parse error

	if(0 == strncmp(eventType, STR_TOUCH, strlen(STR_TOUCH)))
	{
		_device_write(g_touch_dev, &in_ev);
	}
	else if(0 == strncmp(eventType, STR_KEY, strlen(STR_KEY)))
	{
		_device_write(g_key_dev, &in_ev);
	}

	return 0;
}

// return 0 for normal case
// return negative value for error case
static int parseHostMessage(msg_t* log, char* msg)
{
	int i;
	int bfind = 0;
	int ret = 0;	// parsing success

	if(log == NULL || msg == NULL)
		return -1;

	// Host message looks like this
	// MSG_TYPE|MSG_LENGTH|MSG_STRING
	// MSG_TYPE is always 3 digits number
	if(msg[3] == '|')
	{
		msg[3] = '\0';
		log->type = atoi(msg);

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
		log->length = atoi(msg);

		if(log->length == 0)
		{
			log->data[0] = '\0';
		}
		else
		{
			if(bfind != 0)
			{
				int msglen;
				msg = msg + i + 1;
				msglen = strlen(msg);

				if(msglen == log->length)
				{
					strcpy(log->data, msg);
					log->data[log->length] = '\0';
				}
//				else if(msglen > log->length)
//				{
//					strncpy(log->data, msg, log->length);
//					log->data[log->length] = '\0';
//				}
				else
				{
					ret = -1;	// parsing error
				}
			}
			else
			{
				ret = -1;	// parsing error
			}
		}
	}
	else
	{
		ret = -1;	// parsing error
	}

	return ret;
}

// return 0 if normal case
// return plus value if non critical error occur
// return minus value if critical error occur
static int hostMessageHandler(msg_t* log)
{
	int ret = 0;
	long flag = 0;
	char *barloc, *tmploc;
	char execPath[PATH_MAX];

	if (log == NULL)
		return 1;

	switch (log->type)
	{
	case MSG_REPLAY:
		sendACKStrToHost(MSG_OK, NULL);
		parseDeviceMessage(log);
		break;
	case MSG_VERSION:
		if(strcmp(PROTOCOL_VERSION, log->data) != 0)
		{
			sendACKCodeToHost(MSG_NOTOK, ERR_WRONG_PROTOCOL_VERSION);
		}
		else
		{
			sendACKStrToHost(MSG_OK, NULL);
		}
		break;
	case MSG_START:
		LOGI("MSG_START handling : %s\n", log->data);
		if(log->length == 0)
		{
			sendACKCodeToHost(MSG_NOTOK, ERR_WRONG_MESSAGE_DATA);
			return -1;		// wrong message format
		}

		// parsing for host start status
		tmploc  = log->data;
		barloc = strchr(tmploc, '|');
		if(barloc == NULL)
		{
			sendACKCodeToHost(MSG_NOTOK, ERR_WRONG_MESSAGE_FORMAT);
			return -1;		// wrong message format
		}

		// parsing for target launch option flag
		tmploc = barloc + 1;
		barloc = strchr(tmploc, '|');
		if(barloc != NULL)
		{
			while(tmploc < barloc)
			{
				flag = (flag * 10) + (*tmploc - '0');
				tmploc++;
			}
		}
		else
		{
			sendACKCodeToHost(MSG_NOTOK, ERR_WRONG_MESSAGE_FORMAT);
			return -1;	// wrong message format
		}
		LOGI("launch flag : %lx\n", flag);

		// parsing for application package name
		tmploc = barloc + 1;
		strcpy(manager.appPath, tmploc);

		get_executable(manager.appPath, execPath, PATH_MAX); // get exact app executable file name
		LOGI("executable app path %s\n", manager.appPath);

#ifdef RUN_APP_LOADER
		kill_app(manager.appPath);
#else
		kill_app(execPath);
#endif

		{
			char command[PATH_MAX];

			//save app install path
			mkdir(DA_WORK_DIR, 0775);
			sprintf(command,
					"%s -Wwi %s | grep DW_AT_comp_dir > %s", DA_READELF_PATH,
					execPath, DA_INSTALL_PATH);
			LOGI("appInstallCommand %s\n", command);
			system(command);

			sprintf(command,
					"%s -h %s | grep Type | cut -d\" \" -f33 > %s", DA_READELF_PATH,
					execPath, DA_BUILD_OPTION);
			LOGI("appInstallCommand %s\n", command);
			system(command);

			if(startProfiling(flag) < 0)
			{
				sendACKCodeToHost(MSG_NOTOK, ERR_CANNOT_START_PROFILING);
				return -1;
			}
		}
		sendACKStrToHost(MSG_OK, NULL);
		break;
	case MSG_STOP:
		LOGI("MSG_STOP handling\n");
		sendACKStrToHost(MSG_OK, NULL);
		terminate_all();
		break;
	case MSG_OPTION:
		if(log->length > 0)
		{
			int i;
			msg_t sendlog;
			manager.config_flag = atoi(log->data);
			sendACKStrToHost(MSG_OK, NULL);

			LOGI("MSG_OPTION : str(%s), flag(%x)\n", log->data, manager.config_flag);

			sendlog.type = MSG_OPTION;
			sendlog.length = sprintf(sendlog.data, "%u", manager.config_flag);

			for(i = 0; i < MAX_TARGET_COUNT; i++)
			{
				if(manager.target[i].socket != -1)
				{
					send(manager.target[i].socket, &sendlog, sizeof(sendlog.type) + sizeof(sendlog.length) + sendlog.length, MSG_NOSIGNAL);
				}
			}
		}
		else
		{
			sendACKCodeToHost(MSG_NOTOK, ERR_WRONG_MESSAGE_DATA);
			ret = 1;
		}
		break;
	case MSG_ISALIVE:
		sendACKStrToHost(MSG_OK, NULL);
		break;
	default:
		LOGW("Unknown msg\n");
		sendACKCodeToHost(MSG_NOTOK, ERR_WRONG_MESSAGE_TYPE);
		ret = 1;
		break;
	}

	return ret;
}

// ========================================================================================
// socket and event_fd handling functions
// ========================================================================================

// return 0 if normal case
// return plus value if non critical error occur
// return minus value if critical error occur
static int deviceEventHandler(input_dev* dev, int input_type)
{
	int ret = 0;
	struct input_event in_ev;
	msg_t log;

	if(input_type == INPUT_ID_TOUCH)
	{
		//touch read
		read(dev->fd, &in_ev, sizeof(struct input_event));
		log.type = MSG_RECORD;
		log.length = sprintf(log.data, "%s`,%s`,%ld`,%ld`,%hu`,%hu`,%u",
				STR_TOUCH, dev->fileName, in_ev.time.tv_sec,
				in_ev.time.tv_usec, in_ev.type, in_ev.code, in_ev.value);
		sendDataToHost(&log);
	}
	else if(input_type == INPUT_ID_KEY)
	{
		//key read
		read(dev->fd, &in_ev, sizeof(struct input_event));
		log.type = MSG_RECORD;
		log.length = sprintf(log.data, "%s`,%s`,%ld`,%ld`,%hu`,%hu`,%u",
				STR_KEY, dev->fileName, in_ev.time.tv_sec,
				in_ev.time.tv_usec, in_ev.type, in_ev.code, in_ev.value);
		sendDataToHost(&log);
	}
	else
	{
		LOGW("unknown input_type\n");
		ret = 1;
	}

	return ret;
}

// return 0 if normal case
// return plus value if non critical error occur
// return minus value if critical error occur
// return -11 if all target process closed
static int targetEventHandler(int epollfd, int index, uint64_t msg)
{
	msg_t log;

	if(msg & EVENT_PID)
	{
		if(index == 0)		// assume index 0 is main application process
		{
			int base_address;
			char tempBuff[DA_MSG_MAX];
			char tempBuff2[DA_MSG_MAX];
			char tempPath[PATH_MAX];

			get_executable(manager.appPath, tempPath, PATH_MAX);
			if(realpath(tempPath, tempBuff) == NULL)
			{
				LOGW("Failed to get realpath of app\n");
				strcpy(tempBuff, tempPath);
			}

			sprintf(tempPath, "/proc/%d/maps", manager.target[index].pid);
			sprintf(tempBuff2, "cat %s | grep %s | cut -d\"-\" -f1 > %s",
					tempPath, tempBuff, DA_BASE_ADDRESS);
			LOGI("base address command is %s\n", tempBuff2);

			do {
				if(access(tempPath, F_OK) != 0)
					return -1;
				if(is_same_app_process(manager.appPath, manager.target[index].pid) == 0)
					return -1;

				system(tempBuff2);
				if(get_app_base_address(&base_address) == 1)
					break;
				sleep(0);
			} while(1);

			tempPath[0] = '\0';
			get_app_install_path(tempPath, PATH_MAX);
			get_device_info(tempBuff, DA_MSG_MAX);
			log.type = MSG_DEVICE;
			if (strlen(tempPath) > 0)
			{
				get_executable(manager.appPath, tempBuff2, DA_MSG_MAX);
				log.length = sprintf(log.data, "%s`,%d`,%Lu`,%d`,%u`,%d`,%s/%s", tempBuff,
						manager.target[index].pid, manager.target[index].starttime,
						is_app_built_pie(), base_address, get_app_type(manager.appPath),
						tempPath, get_app_name(tempBuff2));
			}
			else
			{
				log.length = sprintf(log.data, "%s`,%d`,%Lu`,%d`,%u`,%d`,", tempBuff,
						manager.target[index].pid, manager.target[index].starttime,
						is_app_built_pie(), base_address, get_app_type(manager.appPath));
			}

			LOGI("%s\n", log.data);
		}
		else
		{
			log.type = MSG_PID;
			log.length = sprintf(log.data, "%d`,%Lu", manager.target[index].pid, manager.target[index].starttime);
		}

		manager.target[index].initial_log = 1;
		sendDataToHost(&log);
	}

	if(msg & EVENT_STOP || msg & EVENT_ERROR)
	{
		LOGI("target close, socket(%d), pid(%d) : (remaining %d target)\n",
				manager.target[index].socket, manager.target[index].pid, manager.target_count - 1);

		terminate_target(index);
		epoll_ctl(epollfd, EPOLL_CTL_DEL, manager.target[index].event_fd, NULL);
		setEmptyTargetSlot(index);
		if (0 == __sync_sub_and_fetch(&manager.target_count, 1))	// all target client are closed
		{
			log.type = MSG_TERMINATE;
			log.length = 0;
			log.data[0] = '\0';
			sendDataToHost(&log);
			return -11;
		}
	}

	return 0;
}

// return 0 if normal case
// return plus value if non critical error occur
// return minus value if critical error occur
static int targetServerHandler(int efd)
{
	msg_t log;
	struct epoll_event ev;

	int index = getEmptyTargetSlot();
	if(index == MAX_TARGET_COUNT)
	{
		LOGW("Max target number(8) reached, no more target can connected\n");
		return 1;
	}

	manager.target[index].socket = accept(manager.target_server_socket, NULL, NULL);

	if(manager.target[index].socket >= 0)	// accept succeed
	{
		// set smack attribute for certification
		fsetxattr(manager.target[index].socket, "security.SMACK64IPIN", "*", 1, 0);
		fsetxattr(manager.target[index].socket, "security.SMACK64IPOUT", "*", 1, 0);

		// send config message to target process
		log.type = MSG_OPTION;
		log.length = sprintf(log.data, "%u", manager.config_flag);
		send(manager.target[index].socket, &log, sizeof(log.type) + sizeof(log.length) + log.length, MSG_NOSIGNAL);

		// make event fd
		manager.target[index].event_fd = eventfd(0, EFD_NONBLOCK);
		if(manager.target[index].event_fd == -1)
		{
			// fail to make event fd
			LOGE("fail to make event fd for socket (%d)\n", manager.target[index].socket);
			goto TARGET_CONNECT_FAIL;
		}

		// add event fd to epoll list
		ev.events = EPOLLIN;
		ev.data.fd = manager.target[index].event_fd;
		if(epoll_ctl(efd, EPOLL_CTL_ADD, manager.target[index].event_fd, &ev) < 0)
		{
			// fail to add event fd
			LOGE("fail to add event fd to epoll list for socket (%d)\n", manager.target[index].socket);
			goto TARGET_CONNECT_FAIL;
		}

		// make recv thread for target
		if(makeRecvThread(index) != 0)
		{
			// fail to make recv thread
			LOGE("fail to make recv thread for socket (%d)\n", manager.target[index].socket);
			epoll_ctl(efd, EPOLL_CTL_DEL, manager.target[index].event_fd, NULL);
			goto TARGET_CONNECT_FAIL;
		}

		LOGI("target connected = %d(running %d target)\n",
				manager.target[index].socket, manager.target_count + 1);

		manager.target_count++;
		return 0;
	}
	else	// accept error
	{
		LOGE("Failed to accept at target server socket\n");
	}

TARGET_CONNECT_FAIL:
	if(manager.target_count == 0)	// if this connection is main connection
	{
		return -1;
	}
	else	// if this connection is not main connection then ignore process by error
	{
		setEmptyTargetSlot(index);
		return 1;
	}
}

// return 0 if normal case
// return plus value if non critical error occur
// return minus value if critical error occur
static int hostServerHandler(int efd)
{
	static int hostserverorder = 0;
	int csocket;
	struct epoll_event ev;

	if(hostserverorder > 1)	// control and data socket connected already
		return 1;			// ignore

	csocket = accept(manager.host_server_socket, NULL, NULL);

	if(csocket >= 0)		// accept succeed
	{
		ev.events = EPOLLIN;
		ev.data.fd = csocket;
		if(epoll_ctl(efd, EPOLL_CTL_ADD, csocket, &ev) < 0)
		{
			// consider as accept fail
			LOGE("Failed to add socket fd to epoll list\n");
			close(csocket);
			return -1;
		}

		if(hostserverorder == 0)
		{
			manager.host.control_socket = csocket;
			unlink_portfile();
			LOGI("host control socket connected = %d\n", csocket);
		}
		else
		{
			manager.host.data_socket = csocket;
			LOGI("host data socket connected = %d\n", csocket);
		}

		hostserverorder++;
		return 0;
	}
	else	// accept error
	{
		LOGE("Failed to accept from host server socket\n");
		return -1;
	}
}

// return 0 if normal case
// return plus value if non critical error occur
// return minus value if critical error occur
// return -11 if socket closed
static int controlSocketHandler()
{
	ssize_t recvLen;
	char recvBuf[DA_MSG_MAX];
	msg_t log;

	// host log format xxx|length|str
	recvLen = recv(manager.host.control_socket, recvBuf, DA_MSG_MAX, 0);

	if (recvLen > 0)
	{
		recvBuf[recvLen] = '\0';
		LOGI("host sent control msg str(%s)\n", recvBuf);

		if(parseHostMessage(&log, recvBuf) < 0)
		{
			// error to parse host message
			sendACKCodeToHost(MSG_NOTOK, ERR_WRONG_MESSAGE_FORMAT);
			return 1;
		}

		// host msg command handling
		return hostMessageHandler(&log);
	}
	else	// close request from HOST
	{
		return -11;
	}
}

// return 0 for normal case
int daemonLoop()
{
	int			ret = 0;				// return value
	int			i, k;
	ssize_t		recvLen;

	struct epoll_event ev, *events;
	int efd;		// epoll fd
	int numevent;	// number of occured events

	_get_fds(g_key_dev, INPUT_ID_KEY);
	_get_fds(g_touch_dev, INPUT_ID_TOUCH);

	// initialize epoll event pool
	events = (struct epoll_event*) malloc(sizeof(struct epoll_event) * EPOLL_SIZE);
	if(events == NULL)
	{
		LOGE("Out of memory when allocate epoll event pool\n");
		ret = -1;
		goto END_RETURN;
	}
	if((efd = epoll_create(MAX_CONNECT_SIZE)) < 0)
	{
		LOGE("epoll creation error\n");
		ret = -1;
		goto END_EVENT;
	}

	// add server sockets to epoll event pool
	ev.events = EPOLLIN;
	ev.data.fd = manager.host_server_socket;
	if(epoll_ctl(efd, EPOLL_CTL_ADD, manager.host_server_socket, &ev) < 0)
	{
		LOGE("Host server socket epoll_ctl error\n");
		ret = -1;
		goto END_EFD;
	}
	ev.events = EPOLLIN;
	ev.data.fd = manager.target_server_socket;
	if(epoll_ctl(efd, EPOLL_CTL_ADD, manager.target_server_socket, &ev) < 0)
	{
		LOGE("Target server socket epoll_ctl error\n");
		ret = -1;
		goto END_EFD;
	}

	// add device fds to epoll event pool
	ev.events = EPOLLIN;
	for(i = 0; g_key_dev[i].fd != ARRAY_END; i++)
	{
		if(g_key_dev[i].fd >= 0)
		{
			ev.data.fd = g_key_dev[i].fd;
			if(epoll_ctl(efd, EPOLL_CTL_ADD, g_key_dev[i].fd, &ev) < 0)
			{
				LOGE("keyboard device file epoll_ctl error\n");
			}
		}
	}

	ev.events = EPOLLIN;
	for(i = 0; g_touch_dev[i].fd != ARRAY_END; i++)
	{
		if(g_touch_dev[i].fd >= 0)
		{
			ev.data.fd = g_touch_dev[i].fd;
			if(epoll_ctl(efd, EPOLL_CTL_ADD, g_touch_dev[i].fd, &ev) < 0)
			{
				LOGE("touch device file epoll_ctl error\n");
			}
		}
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
			// check for request from event fd
			for(k = 0; k < MAX_TARGET_COUNT; k++)
			{
				if(manager.target[k].socket != -1 &&
						events[i].data.fd == manager.target[k].event_fd)
				{
					uint64_t u;
					recvLen = read(manager.target[k].event_fd, &u, sizeof(uint64_t));
					if(recvLen != sizeof(uint64_t))
					{
						// maybe closed, but ignoring is more safe then removing fd from epoll list
					}
					else
					{
						if(-11 == targetEventHandler(efd, k, u))
						{
							LOGI("all target process is closed\n");
							terminate_all();
							ret = 0;
							goto END_EFD;
						}
					}
					break;
				}
			}

			if(k != MAX_TARGET_COUNT)
				continue;

			// check for request from device fd
			for(k = 0; g_touch_dev[k].fd != ARRAY_END; k++)
			{
				if(g_touch_dev[k].fd >= 0 && 
						events[i].data.fd == g_touch_dev[k].fd)
				{
					if(deviceEventHandler(&g_touch_dev[k], INPUT_ID_TOUCH) < 0)
					{
						terminate_error("Internal DA framework error, Please re-run the profiling.", 1);
						ret = -1;
						goto END_EFD;
					}
					break;
				}
			}

			if(g_touch_dev[k].fd != ARRAY_END)
				continue;

			for(k = 0; g_key_dev[k].fd != ARRAY_END; k++)
			{
				if(g_key_dev[k].fd >= 0 && 
						events[i].data.fd == g_key_dev[k].fd)
				{
					if(deviceEventHandler(&g_key_dev[k], INPUT_ID_KEY) < 0)
					{
						terminate_error("Internal DA framework error, Please re-run the profiling.", 1);
						ret = -1;
						goto END_EFD;
					}
					break;
				}
			}

			if(g_key_dev[k].fd != ARRAY_END)
				continue;

			// connect request from target
			if(events[i].data.fd == manager.target_server_socket)
			{
				if(targetServerHandler(efd) < 0)	// critical error
				{
					terminate_error("Internal DA framework error, Please re-run the profiling.", 1);
					ret = -1;
					goto END_EFD;
				}
			}
			// connect request from host
			else if(events[i].data.fd == manager.host_server_socket)
			{
				int result = hostServerHandler(efd);
				if(result < 0)
				{
					terminate_error("Internal DA framework error, Please re-run the profiling.", 1);
					ret = -1;
					goto END_EFD;
				}
			}
			// control message from host
			else if(events[i].data.fd == manager.host.control_socket)
			{
				int result = controlSocketHandler();
				if(result == -11)	// socket close
				{
					// close target and host socket and quit
					LOGI("host close = %d\n", manager.host.control_socket);
					terminate_all();
					ret = 0;
					goto END_EFD;
				}
				else if(result < 0)
				{
					terminate_error("Internal DA framework error, Please re-run the profiling.", 1);
					ret = -1;
					goto END_EFD;
				}
			}
			else if(events[i].data.fd == manager.host.data_socket)
			{
				char recvBuf[32];
				recvLen = recv(manager.host.data_socket, recvBuf, 32, MSG_DONTWAIT);
				if(recvLen == 0)
				{	// close data socket
					epoll_ctl(efd, EPOLL_CTL_DEL, manager.host.data_socket, NULL);
					close(manager.host.data_socket);
					manager.host.data_socket = -1;
				}
	
				LOGW("host message from data socket %d\n", recvLen);
			}
			// unknown socket
			else
			{
				// never happened
				LOGW("Unknown socket fd (%d)\n", events[i].data.fd);
			}
		}
	}

END_EFD:
	close(efd);
END_EVENT:
	free(events);
END_RETURN:
	return ret;
}
