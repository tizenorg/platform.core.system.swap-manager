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
#include <sys/timerfd.h>	// for timerfd
#include <unistd.h>			// for access, sleep

#include <ctype.h>

#ifndef LOCALTEST
#include <attr/xattr.h>		// for fsetxattr
#include <sys/smack.h>
#endif

#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>
#include "daemon.h"
#include "sys_stat.h"
#include "utils.h"
#include "da_protocol.h"
#include "da_data.h"
#include "debug.h"

#define DA_WORK_DIR				"/home/developer/sdk_tools/da/"
#define DA_READELF_PATH			"/home/developer/sdk_tools/da/readelf"
#define SCREENSHOT_DIR			"/tmp/da"

#define EPOLL_SIZE				10
#define MAX_CONNECT_SIZE		12
#define MAX_APP_LAUNCH_TIME		6

#define MAX_DEVICE				10
#define MAX_FILENAME			128
#define BUF_SIZE				1024
#define ARRAY_END				(-11)

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
					dev[count].fd = open(dev[count].fileName, O_RDWR | O_NONBLOCK);
					count++;
				}
			}
		}

		closedir(dp);
	}
	dev[count].fd = ARRAY_END;	// end of input_dev array
}

//static
void _device_write(input_dev *dev, struct input_event* in_ev)
{
	int i;
	for(i = 0; dev[i].fd != ARRAY_END; i++)
	{
		if(dev[i].fd >= 0)
		{
			write(dev[i].fd, in_ev, sizeof(struct input_event));
			LOGI("write(%d, %d, %d)\n",dev[i].fd, (int)in_ev, sizeof(struct input_event));
		}
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

//static
int sendACKCodeToHost(enum HostMessageType resp, int msgcode)
{
	// FIXME:
	//disabled string protocol
	return 0;
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

static int exec_app(const struct app_info_t *app_info)
{
	int res = 0;

	switch (app_info->app_type) {
	case APP_TYPE_TIZEN:
		kill_app(app_info->exe_path);
		if (exec_app_tizen(app_info->app_id, app_info->exe_path)) {
			LOGE("Cannot exec tizen app %s\n", app_info->app_id);
			res = -1;
		}
		break;
	case APP_TYPE_RUNNING:
		// TODO: nothing, it's running
		break;
	case APP_TYPE_COMMON:
		kill_app(app_info->exe_path);
		if (exec_app_common(app_info->exe_path)) {
			LOGE("Cannot exec common app %s\n", app_info->exe_path);
			res = -1;
		}
		break;
	default:
		LOGE("Unknown app type %d\n", app_info->app_type);
		res = -1;
		break;
	}

	return res;
}

static void epoll_add_input_events();
static void epoll_del_input_events();

int start_profiling()
{
	const struct app_info_t *app_info = &prof_session.app_info;
	int res = 0;

	// remove previous screen capture files
	remove_indir(SCREENSHOT_DIR);
	mkdir(SCREENSHOT_DIR, 0777);
#ifndef LOCALTEST
	smack_lsetlabel(SCREENSHOT_DIR, "*", SMACK_LABEL_ACCESS);
#endif

	if (IS_OPT_SET(FL_CPU | FL_MEMORY)) {
		if (samplingStart() < 0) {
			LOGE("Cannot start sampling\n");
			res = -1;
			goto exit;
		}
	}

	if (IS_OPT_SET(FL_RECORDING))
		epoll_add_input_events();

	if (exec_app(app_info)) {
		LOGE("Cannot exec app\n");
		res = -1;
		goto recording_stop;
	}

	goto exit;

recording_stop:
	if (IS_OPT_SET(FL_RECORDING))
		epoll_del_input_events();
	if (IS_OPT_SET(FL_CPU | FL_MEMORY))
		samplingStop();

exit:
	return res;
}

void stop_profiling(void)
{
	if (IS_OPT_SET(FL_RECORDING))
		epoll_del_input_events();
	if (IS_OPT_SET(FL_CPU | FL_MEMORY))
		samplingStop();
}

static void reconfigure_recording(struct conf_t conf)
{
	uint64_t old_features = prof_session.conf.use_features;
	uint64_t new_features = conf.use_features;
	uint64_t to_enable = (new_features ^ old_features) & new_features;
	uint64_t to_disable = (new_features ^ old_features) & old_features;

	if (IS_OPT_SET_IN(FL_RECORDING, to_disable)) {
		epoll_del_input_events();
		prof_session.conf.use_features &= ~FL_RECORDING;
	}

	if (IS_OPT_SET_IN(FL_RECORDING, to_enable)) {
		epoll_add_input_events();
		prof_session.conf.use_features |= FL_RECORDING;
	}

}

static int reconfigure_cpu_and_memory(struct conf_t conf)
{
	uint64_t old_features = prof_session.conf.use_features;
	uint64_t new_features = conf.use_features;
	uint64_t to_enable = (new_features ^ old_features) & new_features;
	uint64_t to_disable = (new_features ^ old_features) & old_features;

	prof_session.conf.system_trace_period = conf.system_trace_period;

	if (IS_OPT_SET(FL_CPU | FL_MEMORY))
		samplingStop();

	if (IS_OPT_SET_IN(FL_CPU | FL_MEMORY, to_disable)) {
		prof_session.conf.use_features &= ~(FL_CPU | FL_MEMORY);
		return 0;
	}

	if (IS_OPT_SET_IN(FL_CPU | FL_MEMORY, to_enable)) {
		if (samplingStart() < 0) {
			LOGE("Cannot start sampling\n");
			return -1;
		}
		prof_session.conf.use_features |= (FL_CPU | FL_MEMORY);
	}

	return 0;
}

int reconfigure(struct conf_t conf)
{
	reconfigure_recording(conf);
	if (reconfigure_cpu_and_memory(conf)) {
		LOGE("Cannot reconf cpu and memory\n");
		return -1;
	}

	return 0;
}

// just send stop message to all target process
static void terminate_all_target()
{
	int i;
	ssize_t sendlen;
	msg_target_t sendlog;

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
void terminate_all()
{
	int i;

	terminate_all_target();

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
// TODO: don't send data to host
static void terminate_error(char* errstr, int sendtohost)
{
	terminate_all();
}

#define MAX_EVENTS_NUM 10
static int deviceEventHandler(input_dev* dev, int input_type)
{
	int ret = 0;
	ssize_t size = 0;
	int count = 0;
	struct input_event in_ev[MAX_EVENTS_NUM];
	struct msg_data_t *log;

	if(input_type == INPUT_ID_TOUCH || input_type == INPUT_ID_KEY)
	{
		do {
			size = read(dev->fd, &in_ev[count], sizeof(*in_ev) );
			if (size >0)
				count++;
		} while (count < MAX_EVENTS_NUM && size > 0);

		if (count) {
			LOGI("read %d %s events\n",
			     count,
			     input_type == INPUT_ID_KEY ? STR_KEY : STR_TOUCH);
			log = gen_message_event(in_ev, count, input_type);
			printBuf((char *)log, MSG_DATA_HDR_LEN + log->len);
			write_to_buf(log);
			reset_data_msg(log);
		}
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
	if(msg & EVENT_PID)
	{
		if (index == 0) { // main application
			if (start_replay() != 0) {
				LOGE("Cannot start replay thread\n");
				return -1;
			}
		}
		manager.target[index].initial_log = 1;
	}

	if(msg & EVENT_STOP || msg & EVENT_ERROR)
	{
		LOGI("target close, socket(%d), pid(%d) : (remaining %d target)\n",
		     manager.target[index].socket,
		     manager.target[index].pid,
		     manager.target_count - 1);
		if (index == 0) { // main application
			stop_replay();
		}
		epoll_ctl(epollfd, EPOLL_CTL_DEL, manager.target[index].event_fd, NULL);
		setEmptyTargetSlot(index);
		if (0 == __sync_sub_and_fetch(&manager.target_count, 1)) // all target client are closed
			return -11;
	}

	return 0;
}

// return 0 if normal case
// return plus value if non critical error occur
// return minus value if critical error occur
static int targetServerHandler(int efd)
{
	msg_target_t log;
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
#ifndef LOCALTEST
		// set smack attribute for certification
		fsetxattr(manager.target[index].socket, "security.SMACK64IPIN", "*", 1, 0);
		fsetxattr(manager.target[index].socket, "security.SMACK64IPOUT", "*", 1, 0);
#endif /* LOCALTEST */

		// send config message to target process
		log.type = MSG_OPTION;
		log.length = sprintf(log.data, "%u",
				     prof_session.conf.use_features);
		send(manager.target[index].socket, &log,
		     sizeof(log.type) + sizeof(log.length) + log.length,
		     MSG_NOSIGNAL);

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

		if(manager.app_launch_timerfd >= 0)
		{
			epoll_ctl(efd, EPOLL_CTL_DEL, manager.app_launch_timerfd, NULL);
			close(manager.app_launch_timerfd);
			manager.app_launch_timerfd = -1;
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

static int controlSocketHandler(int efd)
{
	ssize_t recv_len;
	struct msg_t msg_head;
	struct msg_t *msg;
	int res = 0;

	// Receive header
	recv_len = recv(manager.host.control_socket,
		       &msg_head,
		       MSG_CMD_HDR_LEN, 0);
	// error or close request from host
	if (recv_len == -1 || recv_len == 0)
		return -11;
	else {
		msg = malloc(MSG_CMD_HDR_LEN + msg_head.len);
		if (!msg) {
			LOGE("Cannot alloc msg\n");
			sendACKCodeToHost(MSG_NOTOK, ERR_WRONG_MESSAGE_FORMAT);
			return -1;
		}
		msg->id = msg_head.id;
		msg->len = msg_head.len;
		if (msg->len > 0) {
			// Receive payload (if exists)
			recv_len = recv(manager.host.control_socket,
					msg->payload,
					msg->len, MSG_WAITALL);
			if (recv_len == -1)
				return -11;
		}
		res = host_message_handler(msg);
		free(msg);
	}

	return res;
}

static void epoll_add_input_events()
{
	struct epoll_event ev;
	int i;

	// add device fds to epoll event pool
	ev.events = EPOLLIN;
	for (i = 0; g_key_dev[i].fd != ARRAY_END; i++) {
		if (g_key_dev[i].fd >= 0) {
			ev.data.fd = g_key_dev[i].fd;
			if (epoll_ctl(manager.efd,
				      EPOLL_CTL_ADD,
				      g_key_dev[i].fd, &ev) < 0)
				LOGE("keyboard device file epoll_ctl error\n");
		}
	}

	ev.events = EPOLLIN;
	for (i = 0; g_touch_dev[i].fd != ARRAY_END; i++) {
		if (g_touch_dev[i].fd >= 0) {
			ev.data.fd = g_touch_dev[i].fd;
			if (epoll_ctl(manager.efd,
				      EPOLL_CTL_ADD,
				      g_touch_dev[i].fd, &ev) < 0)
				LOGE("touch device file epoll_ctl error\n");
		}
	}
}

static void epoll_del_input_events()
{
	int i;

	// remove device fds from epoll event pool
	for (i = 0; g_key_dev[i].fd != ARRAY_END; i++)
		if (g_key_dev[i].fd >= 0)
			if (epoll_ctl(manager.efd,
				      EPOLL_CTL_DEL,
				      g_key_dev[i].fd, NULL) < 0)
				LOGE("keyboard device file epoll_ctl error\n");

	for (i = 0; g_touch_dev[i].fd != ARRAY_END; i++)
		if (g_touch_dev[i].fd >= 0)
			if (epoll_ctl(manager.efd,
				      EPOLL_CTL_DEL,
				      g_touch_dev[i].fd, NULL) < 0)
				LOGE("touch device file epoll_ctl error\n");
}

// return 0 for normal case
int daemonLoop()
{
	int			ret = 0;				// return value
	int			i, k;
	ssize_t		recvLen;

	struct epoll_event ev, *events;
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
	if((manager.efd = epoll_create(MAX_CONNECT_SIZE)) < 0)
	{
		LOGE("epoll creation error\n");
		ret = -1;
		goto END_EVENT;
	}

	// add server sockets to epoll event pool
	ev.events = EPOLLIN;
	ev.data.fd = manager.host_server_socket;
	if (epoll_ctl(manager.efd, EPOLL_CTL_ADD,
		     manager.host_server_socket, &ev) < 0)
	{
		LOGE("Host server socket epoll_ctl error\n");
		ret = -1;
		goto END_EFD;
	}
	ev.events = EPOLLIN;
	ev.data.fd = manager.target_server_socket;
	if (epoll_ctl(manager.efd, EPOLL_CTL_ADD,
		      manager.target_server_socket, &ev) < 0)
	{
		LOGE("Target server socket epoll_ctl error\n");
		ret = -1;
		goto END_EFD;
	}

	// handler loop
	while (1)
	{
		numevent = epoll_wait(manager.efd, events, EPOLL_SIZE, -1);
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
						if(-11 == targetEventHandler(manager.efd, k, u))
						{
							LOGI("all target process is closed\n");
							continue;
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
				if(targetServerHandler(manager.efd) < 0)	// critical error
				{
					terminate_error("Internal DA framework error, Please re-run the profiling.", 1);
					ret = -1;
					goto END_EFD;
				}
			}
			// connect request from host
			else if(events[i].data.fd == manager.host_server_socket)
			{
				int result = hostServerHandler(manager.efd);
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
				int result = controlSocketHandler(manager.efd);
				if(result == -11)	// socket close
				{
					// close target and host socket and quit
					LOGI("host close = %d\n", manager.host.control_socket);
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
					epoll_ctl(manager.efd, EPOLL_CTL_DEL,
						  manager.host.data_socket,
						  NULL);
					close(manager.host.data_socket);
					manager.host.data_socket = -1;
					// TODO: finish transfer thread
				}
	
				LOGW("host message from data socket %d\n", recvLen);
			}
			// check for application launch timerfd
			else if(events[i].data.fd == manager.app_launch_timerfd)
			{
				// send to host timeout error message for launching application
				terminate_error("Failed to launch application", 1);
				epoll_ctl(manager.efd, EPOLL_CTL_DEL,
					  manager.app_launch_timerfd, NULL);
				close(manager.app_launch_timerfd);
				manager.app_launch_timerfd = -1;
				ret = -1;
				goto END_EFD;
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
	close(manager.efd);
END_EVENT:
	free(events);
END_RETURN:
	return ret;
}
