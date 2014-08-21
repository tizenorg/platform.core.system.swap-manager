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
#include <sys/timerfd.h>	// for timerfd
#include <unistd.h>		// for access, sleep
#include <stdbool.h>

#include <ctype.h>

#include <fcntl.h>

#include <assert.h>

#include <Ecore.h>

#include "daemon.h"
#include "sys_stat.h"
#include "utils.h"
#include "da_protocol.h"
#include "da_inst.h"
#include "da_data.h"
#include "input_events.h"
#include "smack.h"
#include "debug.h"

#define DA_WORK_DIR			"/home/developer/sdk_tools/da/"
#define DA_READELF_PATH			"/home/developer/sdk_tools/da/readelf"
#define SCREENSHOT_DIR			"/tmp/da"

#define MAX_APP_LAUNCH_TIME		60
#define MAX_CONNECT_TIMEOUT_TIME	5*60

uint64_t get_total_alloc_size_by_pid(pid_t pid)
{
	int i;

	for (i = 0; i < MAX_TARGET_COUNT; i++) {
		if (manager.target[i].socket != -1 &&
		    manager.target[i].pid == pid &&
		    manager.target[i].allocmem > 0)
			return manager.target[i].allocmem;
	}
	return 0;
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

static Ecore_Fd_Handler *launch_timer_handler;

//stop application launch timer
static int stop_app_launch_timer()
{
	close(manager.app_launch_timerfd);
	manager.app_launch_timerfd = -1;

	return 0;
}

static Eina_Bool launch_timer_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	LOGE("Failed to launch application\n");
	if (stop_app_launch_timer() < 0)
		LOGE("cannot stop app launch timer\n");

	return ECORE_CALLBACK_CANCEL;
}

//start application launch timer function
static int start_app_launch_timer(int apps_count)
{
	int res = 0;

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
			stop_app_launch_timer();
			res = -1;
		} else {
			launch_timer_handler =
				ecore_main_fd_handler_add(manager.app_launch_timerfd,
							  ECORE_FD_READ,
							  launch_timer_cb,
							  NULL,
							  NULL, NULL);
			if (!launch_timer_handler) {
				LOGE("fail to add app launch timer fd to \n");
				stop_app_launch_timer();
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

static int kill_app_by_info(const struct app_info_t *app_info)
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
	case APP_TYPE_WEB:
		/* do nothing (it is restarted by itself) */
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
	case APP_TYPE_WEB:
		if (exec_app_web(app_info->app_id)) {
			LOGE("Cannot exec web app %s\n", app_info->app_id);
			res = -1;
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

// just send stop message to all target process
static void terminate_all_target()
{
	int i;
	ssize_t sendlen;
	struct msg_target_t sendlog;

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
			if (write_to_buf(msg) != 0)
				LOGE("write to buf fail\n");
			free_msg_data(msg);
		} else {
			LOGI("cannot generate error message\n");
		}
	}
	terminate_all();
}

static Ecore_Fd_Handler *connect_timer_handler;

static Eina_Bool connect_timer_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	terminate_error("no incoming connections", 1);
	close(manager.connect_timeout_timerfd);
	manager.connect_timeout_timerfd = -1;
	LOGE("No connection in %d sec. shutdown.\n",
	     MAX_CONNECT_TIMEOUT_TIME);
	ecore_main_loop_quit();

	return ECORE_CALLBACK_CANCEL;
}

static int launch_timer_start(void)
{
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
			connect_timer_handler =
				ecore_main_fd_handler_add(manager.connect_timeout_timerfd,
							  ECORE_FD_READ,
							  connect_timer_cb,
							  NULL,
							  NULL, NULL);
			if (!connect_timer_handler) {
				LOGE("fail to add app connection timeout timer fd\n");
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

int prepare_profiling(void)
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

int start_profiling(void)
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

	set_label_for_all(SCREENSHOT_DIR);

	if (samplingStart() < 0) {
		LOGE("Cannot start sampling\n");
		res = -1;
		goto exit;
	}

	if (IS_OPT_SET(FL_RECORDING))
		add_input_events();

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
		del_input_events();
	samplingStop();

 exit:
	LOGI("return %d\n", res);
	return res;
}

void stop_profiling(void)
{
	if (IS_OPT_SET(FL_RECORDING))
		del_input_events();
	samplingStop();
}

static void reconfigure_recording(struct conf_t conf)
{
	uint64_t old_features = prof_session.conf.use_features0;
	uint64_t new_features = conf.use_features0;
	uint64_t to_enable = (new_features ^ old_features) & new_features;
	uint64_t to_disable = (new_features ^ old_features) & old_features;

	if (IS_OPT_SET_IN(FL_RECORDING, to_disable)) {
		del_input_events();
		prof_session.conf.use_features0 &= ~FL_RECORDING;
	}

	if (IS_OPT_SET_IN(FL_RECORDING, to_enable)) {
		add_input_events();
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

static Ecore_Fd_Handler *target_handlers[MAX_TARGET_COUNT];

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

static int target_event_stop_handler(int index, uint64_t msg)
{
	LOGI("target close, socket(%d), pid(%d) : (remaining %d target)\n",
	     manager.target[index].socket, manager.target[index].pid,
	     manager.target_count - 1);

	if (index == 0)		// main application
		stop_replay();

	ecore_main_fd_handler_del(target_handlers[index]);

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
static int target_event_handler(int index, uint64_t msg)
{
	int err = 0;
	if (msg & EVENT_PID)
		err = target_event_pid_handler(index, msg);
	if (err)
		return err;

	if (msg & EVENT_STOP || msg & EVENT_ERROR)
		err = target_event_stop_handler(index, msg);

	return err;
}

static Eina_Bool target_event_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	uint64_t u;
	ssize_t recvLen;
	int index = (int)data;

	recvLen = read(manager.target[index].event_fd, &u, sizeof(uint64_t));
	if (recvLen != sizeof(uint64_t)) {
		// maybe closed, but ignoring is more safe then
		// removing fd from event loop
	} else {
		if (-11 == target_event_handler(index, u)) {
			LOGI("all target process is closed\n");
		}
	}

	return ECORE_CALLBACK_RENEW;
}

/**
 * return 0 if normal case
 * return plus value if non critical error occur
 * return minus value if critical error occur
 */
static int targetServerHandler(void)
{
	struct msg_target_t log;

	int index = getEmptyTargetSlot();
	if (index == MAX_TARGET_COUNT) {
		LOGW("Max target number(8) reached, no more target can connected\n");
		return 1;
	}

	manager.target[index].socket =
	    accept4(manager.target_server_socket, NULL, NULL, SOCK_CLOEXEC);

	if (manager.target[index].socket >= 0) {
		/* accept succeed */
		fd_setup_attributes(manager.target[index].socket);

		/* send config message to target process */
		log.type = MSG_OPTION;
		log.length = sprintf(log.data, "%llu",
				     prof_session.conf.use_features0);
		if (0 > send(manager.target[index].socket, &log,
			     sizeof(log.type) + sizeof(log.length) + log.length,
			     MSG_NOSIGNAL))
			LOGE("fail to send data to target index(%d)\n", index);

		/* send current instrument maps */
		send_maps_inst_msg_to(manager.target[index].socket);

		// make event fd
		manager.target[index].event_fd = eventfd(EFD_CLOEXEC, EFD_NONBLOCK);
		if (manager.target[index].event_fd == -1) {
			// fail to make event fd
			LOGE("fail to make event fd for socket (%d)\n",
			     manager.target[index].socket);
			goto TARGET_CONNECT_FAIL;
		}

		target_handlers[index] =
			ecore_main_fd_handler_add(manager.target[index].event_fd,
						  ECORE_FD_READ,
						  target_event_cb,
						  (void *)index,
						  NULL, NULL);
		if (!target_handlers[index]) {
			LOGE("fail to add event fd for socket (%d)\n",
			     manager.target[index].socket);
			goto TARGET_CONNECT_FAIL;
		}

		// make recv thread for target
		if (makeRecvThread(index) != 0) {
			// fail to make recv thread
			LOGE("fail to make recv thread for socket (%d)\n",
			     manager.target[index].socket);
			ecore_main_fd_handler_del(target_handlers[index]);
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

static Ecore_Fd_Handler *host_ctrl_handler;
static Ecore_Fd_Handler *host_data_handler;

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
			if (recv_len == -1) {
				free(msg);
				return -11;
			}
		}
		printBuf((char *)msg, MSG_CMD_HDR_LEN + msg->len);
		res = host_message_handler(msg);
		free(msg);
	}

	return res;
}

static Eina_Bool host_ctrl_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	int result = controlSocketHandler(manager.efd);
	if (result == -11) {
		// socket close
		//if the host disconnected.
		//In all other cases daemon must report an error and continue the loop
		//close connect_timeoutt and host socket and quit
		LOGI("Connection closed. Termination. (%d)\n",
		     manager.host.control_socket);
		manager.host.data_socket = -1; //splice will fail without that
		ecore_main_loop_quit();
	} else if (result < 0) {
		LOGE("Control socket handler.\n");
	}

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool host_data_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	ssize_t recvLen;
	char recvBuf[32];

	recvLen = recv(manager.host.data_socket, recvBuf, 32, MSG_DONTWAIT);
	if (recvLen == 0) {
		// close data socket
		ecore_main_fd_handler_del(host_data_handler);
		close(manager.host.data_socket);
		manager.host.data_socket = -1;
		// TODO: finish transfer thread
	}

	LOGI("host message from data socket %d\n", recvLen);

	return ECORE_CALLBACK_RENEW;
}

// return 0 if normal case
// return plus value if non critical error occur
// return minus value if critical error occur
static int hostServerHandler(void)
{
	static int hostserverorder = 0;
	int csocket;

	if (hostserverorder > 1)	// control and data socket connected already
		return 1;		// ignore

	csocket = accept4(manager.host_server_socket, NULL, NULL, SOCK_CLOEXEC);

	if (csocket >= 0) {
		// accept succeed

		if (hostserverorder == 0) {
			manager.host.control_socket = csocket;
			unlink_portfile();
			LOGI("host control socket connected = %d\n", csocket);
			host_ctrl_handler =
				ecore_main_fd_handler_add(manager.host.control_socket,
							  ECORE_FD_READ,
							  host_ctrl_cb,
							  NULL,
							  NULL, NULL);
			if (!host_ctrl_handler) {
				LOGE("Failed to add host control socket fd\n");
				close(csocket);
				return -1;
			}
		} else {
			manager.host.data_socket = csocket;
			LOGI("host data socket connected = %d\n", csocket);

			host_data_handler =
				ecore_main_fd_handler_add(manager.host.data_socket,
							  ECORE_FD_READ,
							  host_data_cb,
							  NULL,
							  NULL, NULL);
			if (!host_data_handler) {
				LOGE("Failed to add host data socket fd\n");
				close(csocket);
				return -1;
			}
		}

		hostserverorder++;
		return 0;
	} else {
		// accept error
		LOGE("Failed to accept from host server socket\n");
		return -1;
	}
}

static Ecore_Fd_Handler *host_connect_handler;
static Ecore_Fd_Handler *target_connect_handler;

static Eina_Bool host_connect_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	// connect request from host
	int result = hostServerHandler();
	if (result < 0) {
		LOGE("Internal DA framework error (hostServerHandler)\n");
	}

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool target_connect_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	if (targetServerHandler() < 0) {
		// critical error
		terminate_error("Internal DA framework error, "
				"Please re-run the profiling "
				"(targetServerHandler)\n", 1);
	}

	return ECORE_CALLBACK_RENEW;
}

static bool initialize_events(void)
{
	host_connect_handler =
		ecore_main_fd_handler_add(manager.host_server_socket,
					  ECORE_FD_READ,
					  host_connect_cb,
					  NULL,
					  NULL, NULL);
	if (!host_connect_handler) {
		LOGE("Host server socket add error\n");
		return false;
	}

	target_connect_handler =
		ecore_main_fd_handler_add(manager.target_server_socket,
					  ECORE_FD_READ,
					  target_connect_cb,
					  NULL,
					  NULL, NULL);
	if (!target_connect_handler) {
		LOGE("Target server socket add error\n");
		return false;
	}

	return true;
}

// return 0 for normal case
int daemonLoop(void)
{
	int return_value = 0;

	ecore_init();

	if (init_input_events() == -1) {
		return_value = -1;
		goto END_EVENT;
	}

	if (!initialize_events()) {
		return_value = -1;
		goto END_EFD;
	}

	if (launch_timer_start() < 0) {
		LOGE("Launch timer start failed\n");
		return_value = -1;
		goto END_EFD;
	}

	init_prof_session(&prof_session);

	ecore_main_loop_begin();
	ecore_shutdown();

 END_EFD:
	LOGI("close efd\n");
	close(manager.efd);
 END_EVENT:
	return return_value;
}
