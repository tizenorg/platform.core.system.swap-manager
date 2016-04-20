/*
 *  DA manager
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Lyupa Anastasia <a.lyupa@samsung.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "ui_viewer_lib.h"
#include "ui_viewer_utils.h"
#include "ui_viewer_data.h"

static const char socket_name[] = "/tmp/da_ui.socket";

int		g_timerfd = 0;
long		g_total_alloc_size = 0;
pthread_t	g_recvthread_id;

__traceInfo gTraceInfo;

static int createSocket(void);
static int create_recv_thread(void);
static void *recvThread(void __unused *data);

__attribute__((constructor)) void init_ui_viewer(void)
{
	ui_viewer_clean_log();
	ui_viewer_log("constructor started\n");

	if (createSocket() == 0) {
		create_recv_thread();
	}
}

__attribute__((destructor)) void finite_ui_viewer (void)
{
	ui_viewer_log("destructor started\n");

	if (gTraceInfo.socket.daemonSock != -1)
		close(gTraceInfo.socket.daemonSock);
}

static int create_recv_thread(void)
{
	int err = pthread_create(&g_recvthread_id, NULL, recvThread, NULL);

	if (err)
		PRINTMSG("failed to crate recv thread\n");

	return err;
}

// runtime configure the probe option
static void _configure(char* configstr)
{
	gTraceInfo.optionflag = atoll(configstr);
}

void application_exit()
{
	pid_t gpid;
	FILE *f = NULL;
	char buf[MAX_PATH_LENGTH];
	const char manager_name[] = "da_manager";

	gpid = getpgrp();
	snprintf(buf, sizeof(buf), "/proc/%d/cmdline", gpid);
	f = fopen(buf, "r");
	if (f != NULL) {
		fscanf(f, "%" TOSTRING(MAX_PATH_LENGTH) "s", buf);
		fclose(f);
		if (strlen(buf) == strlen(manager_name) &&
		    strncmp(buf, manager_name, sizeof(manager_name)) == 0) {
			PRINTMSG("App termination: EXIT(0)");
			exit(0);
		}
	}

	PRINTMSG("App termination: kill all process group");
	killpg(gpid, SIGKILL);
}

// create socket to daemon and connect
#define MSG_CONFIG_RECV 0x01
#define MSG_TYPE_AND_INFO_RECV 0x02
static int createSocket(void)
{
	char strerr_buf[MAX_PATH_LENGTH];
	ssize_t recvlen;
	int clientLen, ret = 0;
	struct sockaddr_un clientAddr;
	log_t log;

	gTraceInfo.socket.daemonSock =
	  socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (gTraceInfo.socket.daemonSock != -1)	{
		memset(&clientAddr, '\0', sizeof(clientAddr));
		clientAddr.sun_family = AF_UNIX;
		snprintf(clientAddr.sun_path, sizeof(socket_name), "%s", socket_name);

		clientLen = sizeof(clientAddr);
		if (connect(gTraceInfo.socket.daemonSock,
			    (struct sockaddr *)&clientAddr, clientLen) >= 0)
		{
			char buf[64];
			int recved = 0;

			snprintf(buf, sizeof(buf), "%d %d", getpid(), getppid());
			print_log_str(APP_MSG_PID, buf);

			while (((recved & MSG_CONFIG_RECV) == 0) ||
			       ((recved & MSG_TYPE_AND_INFO_RECV) == 0))
			{
				PRINTMSG("wait incoming message %d\n",
					 gTraceInfo.socket.daemonSock);
				recvlen = recv(gTraceInfo.socket.daemonSock, &log,
					       sizeof(log.type) + sizeof(log.length),
					       MSG_WAITALL);

				ui_viewer_log("recv %d\n", recvlen);
				if (recvlen > 0) {
					char *data_buf = NULL;

					if (log.length < 0 ||
					    (unsigned int)log.length > sizeof(log.data)) {
						PRINTERR("Too long message");
						continue;
					}

					data_buf = malloc(log.length);

					if (data_buf == NULL) {
						PRINTERR("cannot allocate buf to recv msg");
						break;
					}

					recvlen = recv(gTraceInfo.socket.daemonSock, data_buf,
						       log.length, MSG_WAITALL);

					if (recvlen != log.length) {
						PRINTERR("Can not get data from daemon sock\n");
						goto free_data_buf;
					}

					if (log.type == APP_MSG_CONFIG) {
						PRINTMSG("APP_MSG_CONFIG");
						_configure(data_buf);
						recved |= MSG_CONFIG_RECV;
					} else if(log.type ==  APP_MSG_TYPE_AND_INFO) {
						PRINTMSG("APP_MSG_TYPE_AND_INFO <%u>", *((uint32_t *)data_buf));
						// do nothing
						recved |= MSG_TYPE_AND_INFO_RECV;
					} else {
						// unexpected case
						PRINTERR("unknown message! %d", log.type);
					}

free_data_buf:
					if (data_buf != NULL)
						free(data_buf);

				} else if (recvlen < 0) {
					close(gTraceInfo.socket.daemonSock);
					gTraceInfo.socket.daemonSock = -1;
					PRINTERR("recv failed with error(%d)\n",
						 recvlen);
					ret = -1;
					application_exit();
					break;
				} else {
					close(gTraceInfo.socket.daemonSock);
					gTraceInfo.socket.daemonSock = -1;
					PRINTERR("closed by other peer\n");
					ret = -1;
					application_exit();
					break;
				}

			}

			PRINTMSG("createSocket connect() success\n");
		} else {
			close(gTraceInfo.socket.daemonSock);
			gTraceInfo.socket.daemonSock = -1;
			strerror_r(errno, strerr_buf, sizeof(strerr_buf));
			PRINTERR("cannot connect to da_manager. err <%s>\n",
				 strerr_buf);
			ret = -1;
		}
	} else {
		strerror_r(errno, strerr_buf, sizeof(strerr_buf));
		PRINTERR("cannot create socket. err <%s>\n", strerr_buf);
		ret = -1;
	}

	PRINTMSG("socket create done with result = %d", ret);
	return ret;
}

static void *recvThread(void __unused *data)
{
	fd_set readfds, workfds;
	int maxfd = 0, rc;
	uint64_t xtime;
	ssize_t recvlen;
	log_t log;
	char *data_buf = NULL;
	sigset_t profsigmask;
	pthread_t hierarchy_thread_id;
	bool hierarchy_thread_running = false;

	if(gTraceInfo.socket.daemonSock == -1)
		return NULL;

	sigemptyset(&profsigmask);
	sigaddset(&profsigmask, SIGPROF);
	pthread_sigmask(SIG_BLOCK, &profsigmask, NULL);

	FD_ZERO(&readfds);
	if(g_timerfd > 0)
	{
		maxfd = g_timerfd;
		FD_SET(g_timerfd, &readfds);
	}
	if(maxfd < gTraceInfo.socket.daemonSock)
		maxfd = gTraceInfo.socket.daemonSock;
	FD_SET(gTraceInfo.socket.daemonSock, &readfds);

	while(1)
	{
		workfds = readfds;
		rc = select(maxfd + 1, &workfds, NULL, NULL, NULL);
		if(rc < 0)
		{
			continue;
		}

		if(g_timerfd > 0 && FD_ISSET(g_timerfd, &workfds))
		{
			recvlen = read(g_timerfd, &xtime, sizeof(xtime));
			if(recvlen > 0)
			{
				log.length = snprintf(log.data, sizeof(log.data), "%ld", g_total_alloc_size) + 1;
				printLog(&log, APP_MSG_ALLOC);
			}
			else
			{
				// read failed
			}
			continue;
		}
		else if(FD_ISSET(gTraceInfo.socket.daemonSock, &workfds))
		{
			recvlen = recv(gTraceInfo.socket.daemonSock, &log,
					sizeof(log.type) + sizeof(log.length), MSG_WAITALL);

			if(recvlen > 0)	// recv succeed
			{

				if (log.length > 0 &&
				    (unsigned int)log.length <= sizeof(log.data)) {
					data_buf = malloc(log.length);
					if (data_buf == NULL) {
						PRINTERR("cannot allocate buf to recv msg");
						break;
					}
					recvlen = recv(gTraceInfo.socket.daemonSock, data_buf,
						log.length, MSG_WAITALL);
					if (recvlen != log.length) {
						PRINTERR("Can not recv data from\n");
						goto free_data_buf;
					}
				}

				if (log.type == APP_MSG_CONFIG) {
					_configure(data_buf);
				} else if(log.type == APP_MSG_STOP) {
					PRINTMSG("APP_MSG_STOP");
					if (data_buf) {
						free(data_buf);
						data_buf = NULL;
					}
					application_exit();
					break;
				} else if(log.type == APP_MSG_TYPE_AND_INFO) {
					PRINTMSG("APP_MSG_TYPE_AND_INFO");
				} else if(log.type == APP_MSG_GET_UI_HIERARCHY) {
					enum ErrorCode err_code = ERR_UNKNOWN;

					print_log_ui_viewer_hierarchy_status(&err_code);

					if (err_code != ERR_NO) {
						PRINTERR("APP_MSG_GET_UI_HIERARCHY error <%d>", err_code);
						continue;
					}

					if(log.length > 0) {
						Eina_Bool rendering;

						hierarchy_thread_running = false;
						rendering = (enum rendering_option_t)*((uint8_t *)data_buf);
						PRINTMSG("APP_MSG_GET_UI_HIERARCHY, rendering option <%d>", rendering);
						raise_app_window();
						if (pthread_create(&hierarchy_thread_id, NULL, print_log_ui_viewer_info_list,
								   (void*)&rendering)) {
							PRINTERR("failed to crate hierarchy thread");
						} else {
							hierarchy_thread_running = true;
						}
					} else {
						PRINTERR("WRONG APP_MSG_GET_UI_PROPERTIES");
					}

					continue;
				} else if(log.type == APP_MSG_GET_UI_HIERARCHY_CANCEL) {
					PRINTMSG("APP_MSG_GET_UI_HIERARCHY_CANCEL");
					set_hierarchy_status(HIERARCHY_CANCELLED);
					if (hierarchy_thread_running) {
						pthread_join(hierarchy_thread_id, NULL);
						hierarchy_thread_running = false;
					}

					continue;
				} else if(log.type == APP_MSG_GET_UI_SCREENSHOT) {
					if(log.length > 0) {
						Evas_Object *obj;

						obj = (Evas_Object*)(unsigned long)*((uint64_t *)data_buf);
						PRINTMSG("APP_MSG_GET_UI_SCREENSHOT <0x%lx>", obj);
						raise_app_window();
						print_log_ui_obj_screenshot(obj);
					} else {
						PRINTERR("WRONG APP_MSG_GET_UI_SCREENSHOT");
					}

					continue;
				} else {
					PRINTERR("recv unknown message. id = (%d)", log.type);
				}

free_data_buf:
				if (data_buf) {
					free(data_buf);
					data_buf = NULL;
				}

			}
			else if(recvlen == 0)	// closed by other peer
			{
				close(gTraceInfo.socket.daemonSock);
				gTraceInfo.socket.daemonSock = -1;
				break;
			}
			else	// recv error
			{
				PRINTERR("recv failed in recv thread with error(%d)", recvlen);
				continue;
			}
		}
		else	// unknown case
		{
			continue;
		}
	}

	if (data_buf)
		free(data_buf);

	return NULL;
}
