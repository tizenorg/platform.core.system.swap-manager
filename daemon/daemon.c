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
#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdlib.h>		// for realpath
#include <string.h>		// for strtok, strcpy, strncpy
#include <limits.h>		// for realpath
#include <inttypes.h>

#include <errno.h>		// for errno
#include <sys/types.h>		// for accept, mkdir, opendir, readdir
#include <sys/socket.h>		// for accept
#include <sys/stat.h>		// for mkdir
#include <sys/eventfd.h>	// for eventfd
#include <sys/epoll.h>		// for epoll apis
#include <sys/timerfd.h>	// for timerfd
#include <unistd.h>		// for access, sleep
#include <stdbool.h>

#include <ctype.h>

#include <attr/xattr.h>		// for fsetxattr
#include <sys/smack.h>

#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>

#include <assert.h>

#include "daemon.h"
#include "sys_stat.h"
#include "utils.h"
#include "da_protocol.h"
#include "da_inst.h"
#include "da_data.h"
#include "debug.h"

#define DA_WORK_DIR			"/home/developer/sdk_tools/da/"
#define DA_READELF_PATH			"/home/developer/sdk_tools/da/readelf"
#define SCREENSHOT_DIR			"/tmp/da"

#define EPOLL_SIZE			10
#define MAX_APP_LAUNCH_TIME		60
#define MAX_CONNECT_TIMEOUT_TIME	5*60

#define MAX_DEVICE			10
#define MAX_FILENAME			128
#define BUF_SIZE			1024
#define ARRAY_END			(-11)

input_dev g_key_dev[MAX_DEVICE];
input_dev g_touch_dev[MAX_DEVICE];

const char *input_key_devices[] = {
	/* target: Emulator, kernel: 3.4, all buttons */
	"Maru Virtio Hwkey",
	/* target: M0, kernel: 3.0, buttons: volume +/-, home, power */
	"gpio-keys",
	/* target: M0, kernel: 3.0, buttons: menu, back */
	"melfas-touchkey",
	/* target: M0, kernel: 3.10, buttons: menu, back */
	"MELFAS MCS Touchkey",
	/* target: M0, kernel: 3.10, buttons: volume +/-, home, power */
	"gpio-keys.5",
	NULL			//array tail
};

const char *input_touch_devices[] = {
	/* target: Emulator, kernel: 3.0 */
	"Maru Virtio Touchscreen",
	/* target: M0, kernel: 3.0 */
	"sec_touchscreen",
	/* target: M0, kernel: 3.10 */
	"MELPAS MMS114 Touchscreen",
	NULL				//array tail
};


static int check_input(char *inputname, int input_id)
{
	int ret = -1;
	FILE *cmd_fp = NULL;
	char buffer[BUF_SIZE];
	char command[MAX_FILENAME];
	char **name_arr;
	size_t bytes_count;

	sprintf(command, "/sys/class/input/%s/device/name", inputname);
	// run command
	cmd_fp = fopen(command, "r");
	if (cmd_fp == NULL)
		goto exit;

	buffer[0] = '\0';
	bytes_count = fread(buffer, 1, BUF_SIZE, cmd_fp);
	if (bytes_count <= 1) {
		LOGE("Failed to read input_id\n");
		goto exit;
	} else {
		buffer[bytes_count - 1] = '\0';
	}

	if (input_id == INPUT_ID_KEY)
		name_arr = input_key_devices;
	else if (input_id == INPUT_ID_TOUCH)
		name_arr = input_touch_devices;
	else
		goto exit;

	while (*name_arr != NULL) {
		if (strcmp(buffer, *name_arr) == 0) {
			ret = 0;
			goto exit;
		}
		name_arr++;
	}

exit:
	if (cmd_fp != NULL)
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

	if (dp == NULL)
		goto exit;

	while ((d = readdir(dp)) != NULL) {
		if (!strncmp(d->d_name, "event", 5)) {
			// start with "event"
			// event file
			if (!check_input(d->d_name, input_id)) {
				sprintf(dev[count].fileName,
					"/dev/input/%s", d->d_name);
				dev[count].fd = open(dev[count].fileName,
						     O_RDWR | O_NONBLOCK);
				count++;
			}
		}
	}

	closedir(dp);

exit:
	dev[count].fd = ARRAY_END;	// end of input_dev array
}

//static
void _device_write(input_dev *dev, struct input_event *in_ev)
{
	int i;
	for (i = 0; dev[i].fd != ARRAY_END; i++) {
		if (dev[i].fd >= 0) {
			write(dev[i].fd, in_ev, sizeof(struct input_event));
			LOGI("write(%d, %d, %d)\n",
			     dev[i].fd, (int)in_ev, sizeof(struct input_event));
		}
	}
}

uint64_t get_total_alloc_size()
{
	int i;
	uint64_t allocsize = 0;

	for (i = 0; i < MAX_TARGET_COUNT; i++) {
		if (manager.target[i].socket != -1 &&
		    manager.target[i].allocmem > 0)
			allocsize += manager.target[i].allocmem;
	}
	return allocsize;
}

static int getEmptyTargetSlot()
{
	int i;
	for (i = 0; i < MAX_TARGET_COUNT; i++) {
		if (manager.target[i].socket == -1)
			break;
	}

	return i;
}

static void setEmptyTargetSlot(int index)
{
	if (index >= 0 && index < MAX_TARGET_COUNT) {
		manager.target[index].pid = -1;
		manager.target[index].recv_thread = -1;
		manager.target[index].allocmem = 0;
		manager.target[index].initial_log = 0;
		if (manager.target[index].event_fd != -1)
			close(manager.target[index].event_fd);
		manager.target[index].event_fd = -1;
		if (manager.target[index].socket != -1)
			close(manager.target[index].socket);
		manager.target[index].socket = -1;
	}
}

// =============================================================================
// start and terminate control functions
// =============================================================================

//start application launch timer function
static int start_app_launch_timer(int apps_count)
{
	int res = 0;
	struct epoll_event ev;

	assert(apps_count >= 0 && "negative apps count");

	if (apps_count == 0)
		return res;

	manager.app_launch_timerfd =
	    timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
	if (manager.app_launch_timerfd > 0) {
		struct itimerspec ctime;
		ctime.it_value.tv_sec = MAX_APP_LAUNCH_TIME * apps_count;
		ctime.it_value.tv_nsec = 0;
		ctime.it_interval.tv_sec = 0;
		ctime.it_interval.tv_nsec = 0;
		if (timerfd_settime(manager.app_launch_timerfd, 0, &ctime, NULL) < 0) {
			LOGE("fail to set app launch timer\n");
			close(manager.app_launch_timerfd);
			manager.app_launch_timerfd = -1;
			res = -1;
		} else {
			// add event fd to epoll list
			ev.events = EPOLLIN;
			ev.data.fd = manager.app_launch_timerfd;
			if (epoll_ctl(manager.efd, EPOLL_CTL_ADD,
				      manager.app_launch_timerfd, &ev) < 0) {
				// fail to add event fd
				LOGE("fail to add app launch timer fd to epoll list\n");
				close(manager.app_launch_timerfd);
				manager.app_launch_timerfd = -1;
				res = -2;
			} else {
				LOGI("application launch time started\n");
			}
		}
	} else {
		LOGE("cannot create launch timer\n");
		res = -3;
	}

	return res;
}

//stop application launch timer
static int stop_app_launch_timer()
{
	if (0 > epoll_ctl(manager.efd, EPOLL_CTL_DEL,
			  manager.app_launch_timerfd, NULL))
		LOGW("fail to EPOLL DEL of app launch timerfd\n");
	close(manager.app_launch_timerfd);
	manager.app_launch_timerfd = -1;
	return 0;
}

static inline void inc_apps_to_run()
{
	manager.apps_to_run++;
}

static inline void dec_apps_to_run()
{
	if (manager.apps_to_run > 0)
		manager.apps_to_run--;
}

static inline int get_apps_to_run()
{
	return manager.apps_to_run;
}

int kill_app_by_info(const struct app_info_t *app_info)
{
	int res = 0;

	if (app_info == NULL) {
		LOGE("Cannot exec app. app_info is NULL");
		return -1;
	}

	switch (app_info->app_type) {
	case APP_TYPE_TIZEN:
		res = kill_app(app_info->exe_path);
		break;
	case APP_TYPE_RUNNING:
		// TODO: nothing, it's running
		LOGI("already started\n");
		break;
	case APP_TYPE_COMMON:
		res = kill_app(app_info->exe_path);
		break;
	default:
		LOGE("Unknown app type %d\n", app_info->app_type);
		res = -1;
		break;
	}

	return res;
}

static int exec_app(const struct app_info_t *app_info)
{
	int res = 0;

	if (app_info == NULL) {
		LOGE("Cannot exec app. app_info is NULL");
		return -1;
	}

	switch (app_info->app_type) {
	case APP_TYPE_TIZEN:
		if (exec_app_tizen(app_info->app_id, app_info->exe_path)) {
			LOGE("Cannot exec tizen app %s\n", app_info->app_id);
			res = -1;
		} else {
			inc_apps_to_run();
		}
		break;
	case APP_TYPE_RUNNING:
		// TODO: nothing, it's running
		LOGI("already started\n");
		break;
	case APP_TYPE_COMMON:
		if (exec_app_common(app_info->exe_path)) {
			LOGE("Cannot exec common app %s\n", app_info->exe_path);
			res = -1;
		} else {
			inc_apps_to_run();
		}
		break;
	default:
		LOGE("Unknown app type %d\n", app_info->app_type);
		res = -1;
		break;
	}

	LOGI("ret=%d\n", res);
	return res;
}

int launch_timer_start()
{
	static struct epoll_event ev;
	int res = 0;

	manager.connect_timeout_timerfd =
	    timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
	if (manager.connect_timeout_timerfd > 0) {
		struct itimerspec ctime;
		ctime.it_value.tv_sec = MAX_CONNECT_TIMEOUT_TIME;
		ctime.it_value.tv_nsec = 0;
		ctime.it_interval.tv_sec = 0;
		ctime.it_interval.tv_nsec = 0;
		if (timerfd_settime(manager.connect_timeout_timerfd, 0, &ctime, NULL) < 0) {
			LOGE("fail to set connect timeout timer\n");
			close(manager.connect_timeout_timerfd);
			manager.connect_timeout_timerfd = -1;
		} else {
			// add event fd to epoll list
			ev.events = EPOLLIN;
			ev.data.fd = manager.connect_timeout_timerfd;
			if (epoll_ctl(manager.efd, EPOLL_CTL_ADD,
				      manager.connect_timeout_timerfd, &ev) < 0)
			{
				// fail to add event fd
				LOGE("fail to add app connection timeout timer fd to epoll list\n");
				close(manager.connect_timeout_timerfd);
				manager.connect_timeout_timerfd = -1;
			} else {
				LOGI("connection timeout timer started\n");
			}
		}
	} else {
		LOGE("cannot create connection timeout timer\n");
	}

	LOGI("ret=%d\n", res);
	return res;
}

static void epoll_add_input_events();
static void epoll_del_input_events();

int prepare_profiling()
{
	struct app_list_t *app = NULL;
	const struct app_info_t *app_info = NULL;

	app_info = app_info_get_first(&app);
	if (app_info == NULL) {
		LOGE("No app info found\n");
		return -1;
	}

	//all apps
	while (app_info != NULL) {
		if (kill_app_by_info(app_info) != 0) {
			LOGE("kill app failed\n");
			return -1;
		}
		app_info = app_info_get_next(&app);
	}
	//init rw for systeminfo
	//init recv send network systeminfo
	sys_stat_prepare();
	return 0;

}

int start_profiling()
{
	struct app_list_t *app = NULL;
	const struct app_info_t *app_info = NULL;
	int res = 0;

	app_info = app_info_get_first(&app);
	if (app_info == NULL) {
		LOGE("No app info found\n");
		return -1;
	}
	// remove previous screen capture files
	remove_indir(SCREENSHOT_DIR);
	if (mkdir(SCREENSHOT_DIR, 0777) == -1 && errno != EEXIST)
		LOGW("Failed to create directory for screenshot : %s\n",
		     strerror(errno));

	smack_lsetlabel(SCREENSHOT_DIR, "*", SMACK_LABEL_ACCESS);

	if (samplingStart() < 0) {
		LOGE("Cannot start sampling\n");
		res = -1;
		goto exit;
	}

	if (IS_OPT_SET(FL_RECORDING))
		epoll_add_input_events();

	while (app_info != NULL) {
		if (exec_app(app_info)) {
			LOGE("Cannot exec app\n");
			res = -1;
			goto recording_stop;
		}
		app_info = app_info_get_next(&app);
	}

	if (start_app_launch_timer(get_apps_to_run()) < 0) {
		res = -1;
		goto recording_stop;
	}

	goto exit;

 recording_stop:
	if (IS_OPT_SET(FL_RECORDING))
		epoll_del_input_events();
	samplingStop();

 exit:
	LOGI("return %d\n", res);
	return res;
}

void stop_profiling(void)
{
	if (IS_OPT_SET(FL_RECORDING))
		epoll_del_input_events();
	samplingStop();
}

static void reconfigure_recording(struct conf_t conf)
{
	uint64_t old_features = prof_session.conf.use_features0;
	uint64_t new_features = conf.use_features0;
	uint64_t to_enable = (new_features ^ old_features) & new_features;
	uint64_t to_disable = (new_features ^ old_features) & old_features;

	if (IS_OPT_SET_IN(FL_RECORDING, to_disable)) {
		epoll_del_input_events();
		prof_session.conf.use_features0 &= ~FL_RECORDING;
	}

	if (IS_OPT_SET_IN(FL_RECORDING, to_enable)) {
		epoll_add_input_events();
		prof_session.conf.use_features0 |= FL_RECORDING;
	}

}

int reconfigure(struct conf_t conf)
{
	reconfigure_recording(conf);

	samplingStop();
	memcpy(&prof_session.conf, &conf, sizeof(conf));
	if (samplingStart() < 0) {
		LOGE("Cannot start sampling\n");
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

	for (i = 0; i < MAX_TARGET_COUNT; i++) {
		if (manager.target[i].socket != -1) {
			sendlen = send(manager.target[i].socket, &sendlog,
				       sizeof(sendlog.type) +
				       sizeof(sendlog.length), MSG_NOSIGNAL);
			if (sendlen != -1) {
				LOGI("TERMINATE send exit msg (socket %d) "
				     "by terminate_all_target()\n",
				     manager.target[i].socket);
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
	for (i = 0; i < MAX_TARGET_COUNT; i++) {
		if (manager.target[i].recv_thread != -1) {
			LOGI("join recv thread [%d] is started\n", i);
			pthread_join(manager.target[i].recv_thread, NULL);
			LOGI("join recv thread %d. done\n", i);
		}
	}
}

// terminate all profiling by critical error
// TODO: don't send data to host
static void terminate_error(char *errstr, int send_to_host)
{
	LOGE("termination all with err '%s'\n", errstr);
	struct msg_data_t *msg = NULL;
	if (send_to_host != 0) {
		msg = gen_message_error(errstr);
		if (msg) {
			write_to_buf(msg);
			free_msg_data(msg);
		} else {
			LOGI("cannot generate error message\n");
		}
	}
	terminate_all();
}

#define MAX_EVENTS_NUM 10
static int deviceEventHandler(input_dev *dev, int input_type)
{
	int ret = 0;
	ssize_t size = 0;
	int count = 0;
	struct input_event in_ev[MAX_EVENTS_NUM];
	struct msg_data_t *log;

	if (input_type == INPUT_ID_TOUCH || input_type == INPUT_ID_KEY) {
		do {
			size = read(dev->fd, &in_ev[count], sizeof(*in_ev));
			if (size > 0)
				count++;
		} while (count < MAX_EVENTS_NUM && size > 0);

		if (count) {
			LOGI("read %d %s events\n",
			     count,
			     input_type == INPUT_ID_KEY ? STR_KEY : STR_TOUCH);
			log = gen_message_event(in_ev, count, input_type);
			printBuf((char *)log, MSG_DATA_HDR_LEN + log->len);
			write_to_buf(log);
			free_msg_data(log);
		}
	} else {
		LOGW("unknown input_type\n");
		ret = 1;	// it is not error
	}
	return ret;
}

static int target_event_pid_handler(int index, uint64_t msg)
{
	struct app_list_t *app = NULL;
	struct app_info_t *app_info = NULL;
	if (index == 0) {	// main application
		app_info = app_info_get_first(&app);
		if (app_info == NULL) {
			LOGE("No app info found\n");
			return -1;
		}

		while (app_info != NULL) {
			if (is_same_app_process(app_info->exe_path,
						manager.target[index].pid))
				break;
			app_info = app_info_get_next(&app);
		}

		if (app_info == NULL) {
			LOGE("pid %d not found in app list\n",
			     manager.target[index].pid);
			return -1;
		}

		if (start_replay() != 0) {
			LOGE("Cannot start replay thread\n");
			return -1;
		}
	}
	manager.target[index].initial_log = 1;
	return 0;
}

static int target_event_stop_handler(int epollfd, int index, uint64_t msg)
{
	LOGI("target close, socket(%d), pid(%d) : (remaining %d target)\n",
	     manager.target[index].socket, manager.target[index].pid,
	     manager.target_count - 1);

	if (index == 0)		// main application
		stop_replay();

	if (0 > epoll_ctl(epollfd, EPOLL_CTL_DEL,
			  manager.target[index].event_fd, NULL))
		LOGW("fail to EPOLL DEL of event fd(%d)\n", index);

	setEmptyTargetSlot(index);
	// all target client are closed
	if (0 == __sync_sub_and_fetch(&manager.target_count, 1)) {
		LOGI("all targets are stopped\n");
		if (stop_all() != ERR_NO)
			LOGE("Stop failed\n");
		return -11;
	}

	return 0;
}

// return 0 if normal case
// return plus value if non critical error occur
// return minus value if critical error occur
// return -11 if all target process closed
static int target_event_handler(int epollfd, int index, uint64_t msg)
{
	int err = 0;
	if (msg & EVENT_PID)
		err = target_event_pid_handler(index, msg);
	if (err)
		return err;

	if (msg & EVENT_STOP || msg & EVENT_ERROR)
		err = target_event_stop_handler(epollfd, index, msg);

	return err;
}

/**
 * return 0 if normal case
 * return plus value if non critical error occur
 * return minus value if critical error occur
 */
static int targetServerHandler(int efd)
{
	msg_target_t log;
	struct epoll_event ev;

	int index = getEmptyTargetSlot();
	if (index == MAX_TARGET_COUNT) {
		LOGW("Max target number(8) reached, no more target can connected\n");
		return 1;
	}

	manager.target[index].socket =
	    accept(manager.target_server_socket, NULL, NULL);

	if (manager.target[index].socket >= 0) {
		/* accept succeed */
		fd_setup_smack_attributes(manager.target[index].socket);

		/* send config message to target process */
		log.type = MSG_OPTION;
		log.length = sprintf(log.data, "%llu",
				     prof_session.conf.use_features0);
		if (0 > send(manager.target[index].socket, &log,
			     sizeof(log.type) + sizeof(log.length) + log.length,
			     MSG_NOSIGNAL))
			LOGE("fail to send data to target index(%d)\n", index);

		// make event fd
		manager.target[index].event_fd = eventfd(0, EFD_NONBLOCK);
		if (manager.target[index].event_fd == -1) {
			// fail to make event fd
			LOGE("fail to make event fd for socket (%d)\n",
			     manager.target[index].socket);
			goto TARGET_CONNECT_FAIL;
		}

		// add event fd to epoll list
		ev.events = EPOLLIN;
		ev.data.fd = manager.target[index].event_fd;
		if (epoll_ctl(efd, EPOLL_CTL_ADD, manager.target[index].event_fd, &ev) < 0) {
			// fail to add event fd
			LOGE("fail to add event fd to epoll list for socket (%d)\n",
			     manager.target[index].socket);
			goto TARGET_CONNECT_FAIL;
		}

		// make recv thread for target
		if (makeRecvThread(index) != 0) {
			// fail to make recv thread
			LOGE("fail to make recv thread for socket (%d)\n",
			     manager.target[index].socket);
			if (0 > epoll_ctl(efd, EPOLL_CTL_DEL,
					  manager.target[index].event_fd, NULL))
				LOGW("fail to EPOLL DEL of event fd(%d)\n", index);
			goto TARGET_CONNECT_FAIL;
		}

		dec_apps_to_run();

		if ((manager.app_launch_timerfd > 0) && (get_apps_to_run() == 0)) {
			if (stop_app_launch_timer() < 0)
				LOGE("cannot stop app launch timer\n");
		}

		LOGI("target connected = %d(running %d target)\n",
		     manager.target[index].socket, manager.target_count + 1);

		manager.target_count++;
		return 0;
	} else {
		// accept error
		LOGE("Failed to accept at target server socket\n");
	}

 TARGET_CONNECT_FAIL:
	if (manager.target_count == 0) {
		// if this connection is main connection
		return -1;
	} else {
		// if this connection is not main connection then ignore process by error
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

	if (hostserverorder > 1)	// control and data socket connected already
		return 1;		// ignore

	csocket = accept(manager.host_server_socket, NULL, NULL);

	if (csocket >= 0) {
		// accept succeed
		ev.events = EPOLLIN;
		ev.data.fd = csocket;
		if (epoll_ctl(efd, EPOLL_CTL_ADD, csocket, &ev) < 0) {
			// consider as accept fail
			LOGE("Failed to add socket fd to epoll list\n");
			close(csocket);
			return -1;
		}

		if (hostserverorder == 0) {
			manager.host.control_socket = csocket;
			unlink_portfile();
			LOGI("host control socket connected = %d\n", csocket);
		} else {
			manager.host.data_socket = csocket;
			LOGI("host data socket connected = %d\n", csocket);
		}

		hostserverorder++;
		return 0;
	} else {
		// accept error
		LOGE("Failed to accept from host server socket\n");
		return -1;
	}
}

// return plus value if non critical error occur
// return minus value if critical error occur
// return -11 if socket closed

static int controlSocketHandler(int efd)
{
	ssize_t recv_len;
	struct msg_t msg_head;
	struct msg_t *msg;
	int res = 0;

	if (manager.connect_timeout_timerfd >= 0) {
		LOGI("release connect timeout timer\n");
		if (0 > epoll_ctl(efd, EPOLL_CTL_DEL,
				  manager.connect_timeout_timerfd, NULL))
			LOGW("fail to EPOLL DEL of timeout timer fd\n");
		close(manager.connect_timeout_timerfd);
		manager.connect_timeout_timerfd = -1;
	}
	// Receive header
	recv_len = recv(manager.host.control_socket,
			&msg_head, MSG_CMD_HDR_LEN, 0);
	// error or close request from host
	if (recv_len == -1 || recv_len == 0)
		return -11;
	else {
		msg = malloc(MSG_CMD_HDR_LEN + msg_head.len);
		if (!msg) {
			LOGE("Cannot alloc msg\n");
			sendACKToHost(msg_head.id, ERR_WRONG_MESSAGE_FORMAT, 0, 0);
			return -1;
		}
		msg->id = msg_head.id;
		msg->len = msg_head.len;
		if (msg->len > 0) {
			// Receive payload (if exists)
			recv_len = recv(manager.host.control_socket,
					msg->payload, msg->len, MSG_WAITALL);
			if (recv_len == -1)
				return -11;
		}
		printBuf((char *)msg, MSG_CMD_HDR_LEN + msg->len);
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
				      EPOLL_CTL_ADD, g_key_dev[i].fd, &ev) < 0
				      && errno != EEXIST)
				LOGE("keyboard device file epoll_ctl error: %s\n", strerror(errno));
		}
	}

	ev.events = EPOLLIN;
	for (i = 0; g_touch_dev[i].fd != ARRAY_END; i++) {
		if (g_touch_dev[i].fd >= 0) {
			ev.data.fd = g_touch_dev[i].fd;
			if (epoll_ctl(manager.efd,
				      EPOLL_CTL_ADD,
				      g_touch_dev[i].fd, &ev) < 0
				      && errno != EEXIST)
				LOGE("touch device file epoll_ctl error: %s\n", strerror(errno));
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
				      EPOLL_CTL_DEL, g_key_dev[i].fd, NULL) < 0)
				LOGE("keyboard device file epoll_ctl error: %s\n", strerror(errno));

	for (i = 0; g_touch_dev[i].fd != ARRAY_END; i++)
		if (g_touch_dev[i].fd >= 0)
			if (epoll_ctl(manager.efd,
				      EPOLL_CTL_DEL,
				      g_touch_dev[i].fd, NULL) < 0)
				LOGE("touch device file epoll_ctl error: %s\n", strerror(errno));
}

static bool initialize_epoll_events(void)
{
	struct epoll_event ev;

	if ((manager.efd = epoll_create1(0)) < 0) {
		LOGE("epoll creation error\n");
		return false;
	}
	// add server sockets to epoll event pool
	ev.events = EPOLLIN;
	ev.data.fd = manager.host_server_socket;
	if (epoll_ctl(manager.efd, EPOLL_CTL_ADD,
		      manager.host_server_socket, &ev) < 0) {
		LOGE("Host server socket epoll_ctl error\n");
		return false;
	}
	ev.events = EPOLLIN;
	ev.data.fd = manager.target_server_socket;
	if (epoll_ctl(manager.efd, EPOLL_CTL_ADD,
		      manager.target_server_socket, &ev) < 0) {
		LOGE("Target server socket epoll_ctl error\n");
		return false;
	}
	return true;
}

// return 0 for normal case
int daemonLoop()
{
	int return_value = 0;
	struct epoll_event *events = malloc(EPOLL_SIZE * sizeof(*events));

	_get_fds(g_key_dev, INPUT_ID_KEY);
	if (g_key_dev[0].fd == ARRAY_END) {
		LOGE("No key devices found.\n");
		return_value = -1;
		goto END_EVENT;
	}
	_get_fds(g_touch_dev, INPUT_ID_TOUCH);
	if (g_touch_dev[0].fd == ARRAY_END) {
		LOGE("No touch devices found.\n");
		return_value = -1;
		goto END_EVENT;
	}

	if (!events) {
		LOGE("Out of memory when allocate epoll event pool\n");
		return_value = -1;
		goto END_EVENT;
	}
	if (!initialize_epoll_events()) {
		return_value = -1;
		goto END_EFD;
	}

	if (launch_timer_start() < 0) {
		LOGE("Launch timer start failed\n");
		return_value = -1;
		goto END_EFD;
	}

	init_prof_session(&prof_session);

	// handler loop
	while (1) {
		int i, k;
		ssize_t recvLen;
		// number of occured events
		int numevent = epoll_wait(manager.efd, events, EPOLL_SIZE, -1);
		if (numevent <= 0) {
			LOGE("Failed to epoll_wait : num of event(%d), errno(%d)\n", numevent, errno);
			continue;
		}

		for (i = 0; i < numevent; i++) {
			// check for request from event fd
			for (k = 0; k < MAX_TARGET_COUNT; k++) {
				if (manager.target[k].socket != -1 &&
				    events[i].data.fd == manager.target[k].event_fd) {
					uint64_t u;
					recvLen = read(manager.target[k].event_fd, &u, sizeof(uint64_t));
					if (recvLen != sizeof(uint64_t)) {
						// maybe closed, but ignoring is more safe then
						// removing fd from epoll list
					} else {
						if (-11 == target_event_handler(manager.efd, k, u))
							LOGI("all target process is closed\n");
					}
					break;
				}
			}

			if (k != MAX_TARGET_COUNT)
				continue;

			// check for request from device fd
			for (k = 0; g_touch_dev[k].fd != ARRAY_END; k++) {
				if (g_touch_dev[k].fd >= 0 &&
				    events[i].data.fd == g_touch_dev[k].fd) {
					if (deviceEventHandler(&g_touch_dev[k],
					    INPUT_ID_TOUCH) < 0) {
						LOGE("Internal DA framework error, "
						     "Please re-run the profiling (touch dev)\n");
						continue;
					}
					break;
				}
			}

			if (g_touch_dev[k].fd != ARRAY_END)
				continue;

			for (k = 0; g_key_dev[k].fd != ARRAY_END; k++) {
				if (g_key_dev[k].fd >= 0 &&
				    events[i].data.fd == g_key_dev[k].fd) {
					if (deviceEventHandler(&g_key_dev[k], INPUT_ID_KEY) < 0) {
						LOGE("Internal DA framework error, "
						     "Please re-run the profiling (key dev)\n");
						continue;
					}
					break;
				}
			}

			if (g_key_dev[k].fd != ARRAY_END)
				continue;

			// connect request from target
			if (events[i].data.fd == manager.target_server_socket) {
				if (targetServerHandler(manager.efd) < 0) {
					// critical error
					terminate_error("Internal DA framework error, "
							"Please re-run the profiling "
							"(targetServerHandler)\n", 1);
					continue;
				}
			} else if (events[i].data.fd == manager.host_server_socket) {
			// connect request from host
				int result = hostServerHandler(manager.efd);
				if (result < 0) {
					LOGE("Internal DA framework error (hostServerHandler)\n");
					continue;
				}
			} else if (events[i].data.fd == manager.host.control_socket) {
			// control message from host
				int result = controlSocketHandler(manager.efd);
				if (result == -11) {
					// socket close
					//if the host disconnected.
					//In all other cases daemon must report an error and continue the loop
					//close connect_timeoutt and host socket and quit
					LOGI("Connection closed. Termination. (%d)\n",
					      manager.host.control_socket);
					return_value = 0;
					goto END_EFD;
				} else if (result < 0) {
					LOGE("Control socket handler.\n");
				}
			} else if (events[i].data.fd == manager.host.data_socket) {
				char recvBuf[32];
				recvLen = recv(manager.host.data_socket, recvBuf, 32, MSG_DONTWAIT);
				if (recvLen == 0) {
					// close data socket
					if (0 > epoll_ctl(manager.efd,
							  EPOLL_CTL_DEL,
							  manager.host.data_socket,
							  NULL))
						LOGW("fail to EPOLL DEL of host data socket\n");
					close(manager.host.data_socket);
					manager.host.data_socket = -1;
					// TODO: finish transfer thread
				}

				LOGI("host message from data socket %d\n",
				     recvLen);
			} else if (events[i].data.fd == manager.app_launch_timerfd) {
				// check for application launch timerfd
				// send to host timeout error message for launching application
				LOGE("Failed to launch application\n");
				if (stop_app_launch_timer() < 0)
					LOGE("cannot stop app launch timer\n");
				continue;
			} else if (events[i].data.fd == manager.connect_timeout_timerfd) {
				// check for connection timeout timerfd
				// send to host timeout error message for launching application
				terminate_error("no incoming connections", 1);
				if (0 > epoll_ctl(manager.efd, EPOLL_CTL_DEL,
						  manager.connect_timeout_timerfd,
						  NULL))
					LOGW("fail to EPOLL DEL of timeout timer fd\n");
				close(manager.connect_timeout_timerfd);
				manager.connect_timeout_timerfd = -1;
				LOGE("No connection in %d sec. shutdown.\n",
				     MAX_CONNECT_TIMEOUT_TIME);
				goto END_EFD;
			} else {
				// unknown socket
				// never happened
				LOGW("Unknown socket fd (%d)\n",
				     events[i].data.fd);
			}
		}
	}

 END_EFD:
	LOGI("close efd\n");
	close(manager.efd);
 END_EVENT:
	free(events);
	return return_value;
}
