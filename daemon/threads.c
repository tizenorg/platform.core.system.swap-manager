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


#include <stdio.h>
#include <stdlib.h>			// for atoi, atol
#include <string.h>			// for strchr
#include <stdint.h>			// for uint64_t
#include <sys/types.h>		// for recv
#include <sys/socket.h>		// for recv
#include <sys/time.h>		// for setitimer
#include <signal.h>			// for sigemptyset, sigset_t, sigaddset, ...
#include <unistd.h>			// for write
#include <sys/wait.h>

#include "daemon.h"
#include "utils.h"
#include "sys_stat.h"

#include "da_protocol.h"
#include "da_data.h"
#include "da_inst.h"
#include "swap_debug.h"
#include "buffer.h"
#include "input_events.h"

static chsmack(const char *filename)
{
	int res = 1;
	pid_t pid;
	char cmd[1024];
	int status;

	pid = fork();
	switch (pid) {
	case -1:
		/* fail to fork */
		LOGE("cannot fork");
		break;
	case 0:
		/* child */
		snprintf(cmd, sizeof(cmd), "chsmack -a sdbd \"%s\"", filename);
		execl(SHELL_CMD, SHELL_CMD, "-c", cmd, NULL);

		/* exec fail */
		LOGE("exec fail! <%s>\n", cmd);
		break;
	default:
		/* parent */
		waitpid(pid, &status, 0);
		res = 0;
	}

	return res;
}
static void* recvThread(void* data)
{
	struct target *target = data;
	int pass = 0;
	uint64_t event;
	struct msg_target_t log;
	int err;

	// initialize target variable
	target->allocmem = 0;

	for (;;) {
		err = target_recv_msg(target, &log);
		if ((err != 0) || (log.length >= TARGER_MSG_MAX_LEN)) {
			/* disconnect */
			event = EVENT_STOP;
			write(target->event_fd, &event, sizeof(event));
			break;
		}

		if (IS_PROBE_MSG(log.type)) {
			struct msg_data_t *msg_data = (struct msg_data_t *)&log;

			if (write_to_buf(msg_data) != 0)
				LOGE("write to buf fail\n");

			continue;
		}

		log.data[log.length] = '\0';
		if (log.type == APP_MSG_ALLOC) {
			target->allocmem = str_to_int64(log.data);
			continue;		// don't send to host
		} else if (log.type == APP_MSG_PID) {
			LOGI("APP_MSG_PID arrived (pid ppid): %s\n", log.data);

			/* only when first APP_MSG_PID is arrived */
			if (target_get_pid(target) == UNKNOWN_PID) {
				int n;
				pid_t pid, ppid;

				n = sscanf(log.data, "%d %d", &pid, &ppid);
				if (n != 2)
				{
					/**
					 * TODO: complain to host about wrong
					 * pid message send stop message to
					 * main thread
					 */
					event = EVENT_STOP;
					write(target->event_fd, &event,
					      sizeof(uint64_t));
					break;
				}

				/* set pid and ppid */
				target_set_pid(target, pid);
				target_set_ppid(target, ppid);

				/* send event */
				event = EVENT_PID;
				write(target->event_fd, &event, sizeof(uint64_t));
			}
			send_maps_inst_msg_to(target);
			continue;		// don't send to host
		} else if (log.type == APP_MSG_TERMINATE) {
			LOGI("APP_MSG_TERMINATE arrived: pid[%d]\n",
			     target_get_pid(target));

			// send stop message to main thread
			event = EVENT_STOP;
			write(target->event_fd, &event,
			      sizeof(uint64_t));

			break;
		} else if (log.type == APP_MSG_MSG) {
			// don't send to host
			LOGI("EXTRA '%s'\n", log.data);
			continue;
		} else if (log.type == APP_MSG_ERROR) {
			// don't send to host
			LOGE("EXTRA '%s'\n", log.data);
			continue;
		} else if (log.type == APP_MSG_WARNING) {
			// don't send to host
			LOGW("EXTRA '%s'\n", log.data);
			continue;
		} else if (log.type == APP_MSG_IMAGE) {
			/* need chsmak */
			void *p = log.data;
			char *file_name = p;

			if (access(file_name, F_OK) != -1) {
				LOGI("APP_MSG_IMAGE> <%s>\n", file_name);
			} else {
				LOGE("APP_MSG_IMAGE> File not found <%s>\n",
				     file_name);
				continue;
			}

			if (chsmack(file_name) == 0) {
				/* exctract probe message */
				p += strnlen(file_name, PATH_MAX) + 1;
				struct msg_data_t *msg_data = (struct msg_data_t *)(p);

				/* check packed size */
				if (log.length != strnlen(file_name, PATH_MAX) + 1 +
					sizeof(*msg_data) + msg_data->len) {
					LOGE("Bad packed message\n");
					continue;
				}

				if (write_to_buf(msg_data) != 0)
					LOGE("write to buf fail\n");
			} else {
				LOGE("chsmack fail\n");
			}


			continue;
		} else if (log.type == APP_MSG_GET_UI_HIERARCHY) {
			enum ErrorCode err_code = ERR_NO;
			char *file_name = log.data;
			FILE * fp;

			if (access(file_name, F_OK) != -1) {
				LOGI("APP_MSG_GET_UI_HIERARCHY> File: <%s>\n",
				     file_name);
			} else {
				LOGE("APP_MSG_GET_UI_HIERARCHY> File not found <%s>\n",
				     file_name);

				err_code = ERR_WRONG_MESSAGE_DATA;
				goto send_ack;
			}
send_ack:
			sendACKToHost(NMSG_GET_UI_HIERARCHY, err_code, log.data, log.length);

			continue;
		} else if (log.type == APP_MSG_GET_UI_PROPERTIES) {
			enum ErrorCode err_code = ERR_NO;
			char *payload = log.data;
			int payload_size = log.length;

			LOGI("APP_MSG_GET_UI_PROPERTIES> log length = %d\n", log.length);

			if (!payload_size) {
				err_code = ERR_UI_OBJ_NOT_FOUND;
				payload = NULL;
			}

			sendACKToHost(NMSG_GET_UI_PROPERTIES, err_code, payload, payload_size);

			continue;
		} else if (log.type == APP_MSG_GET_UI_RENDERING_TIME) {
			enum ErrorCode err_code = ERR_UNKNOWN;
			char *payload = log.data;
			int payload_size = log.length;

			LOGI("APP_MSG_GET_UI_RENDERING_TIME> log length = %d\n", log.length);

			if (payload_size >= sizeof(uint32_t)) {
				err_code = *(uint32_t*)payload;
				payload += sizeof(uint32_t);
				payload_size -= sizeof(uint32_t);
			}

			sendACKToHost(NMSG_GET_UI_RENDERING_TIME, err_code, payload, payload_size);

			continue;
		}
#ifdef PRINT_TARGET_LOG
		else if (log.type == APP_MSG_LOG) {
			switch(log.data[0])
			{
				case '2':	// UI control creation log
				case '3':	// UI event log
				case '6':	// UI lifecycle log
				case '7':	// screenshot log
				case '8':	// scene transition log
					LOGI("%dclass|%s\n", log.data[0] - '0', log.data);
					break;
				default:
					break;
			}
		} else {
			/* not APP_MSG_LOG and not APP_MSG_IMAGE */
			LOGI("Extra MSG TYPE (%d|%d|%s)\n", log.type, log.length, log.data);
		}
#endif

		/* do not send any message to host until APP_MSG_PID message arrives */
		if(unlikely(pass == 0))
		{
			while(target->initial_log == 0)
			{
				sleep(0);
			}
		}
		pass = 1;
	}

	return NULL;
}

int makeRecvThread(struct target *target)
{
	if (target_start(target, recvThread) < 0) {
		LOGE("Failed to create recv thread\n");
		return -1;
	}

	return 0;
}

static void *samplingThread(void *data)
{
	int err, signo;
	sigset_t waitsigmask;

	LOGI("sampling thread started\n");

	sigemptyset(&waitsigmask);
	sigaddset(&waitsigmask, SIGALRM);
	sigaddset(&waitsigmask, SIGUSR1);

	while (1) {
		err = sigwait(&waitsigmask, &signo);
		if (err != 0) {
			LOGE("Failed to sigwait() in sampling thread\n");
			continue;
		}

		if (signo == SIGALRM) {
			struct system_info_t sys_info;
			struct msg_data_t *msg;

			if (get_system_info(&sys_info) == -1) {
				LOGE("Cannot get system info\n");
				//do not send sys_info because
				//it is corrupted
				continue;
			}

			msg = pack_system_info(&sys_info);
			if (!msg) {
				LOGE("Cannot pack system info\n");
				reset_system_info(&sys_info);
				continue;
			}

			if (write_to_buf(msg) != 0)
				LOGE("write to buf fail\n");

#ifdef THREAD_SAMPLING_DEBUG
			printBuf((char *)msg, MSG_DATA_HDR_LEN + msg->len);
#endif

			free_msg_data(msg);
			reset_system_info(&sys_info);
			flush_buf();
		}
		else if(signo == SIGUSR1)
		{
			LOGI("SIGUSR1 catched\n");
			// end this thread
			break;
		}
		else
		{
			// never happen
			LOGE("This should never happen in sampling thread\n");
		}
	}

	LOGI("sampling thread ended\n");
	return NULL;
}

// return 0 if normal case
// return minus value if critical error
// return plus value if non-critical error
int samplingStart(void)
{
	struct itimerval timerval;
	time_t sec = prof_session.conf.system_trace_period / 1000;
	suseconds_t usec = prof_session.conf.system_trace_period * 1000 %
		1000000;

	if(manager.sampling_thread != -1)	// already started
		return 1;

	if (check_running_status(&prof_session) == 0) {
		LOGI("try to start sampling when running_status is 0\n");
		return 1;
	}

	if(pthread_create(&(manager.sampling_thread), NULL, samplingThread, NULL) < 0)
	{
		LOGE("Failed to create sampling thread\n");
		return -1;
	}

	timerval.it_interval.tv_sec = sec;
	timerval.it_interval.tv_usec = usec;
	timerval.it_value.tv_sec = sec;
	timerval.it_value.tv_usec = usec;
	setitimer(ITIMER_REAL, &timerval, NULL);

	return 0;
}

int samplingStop(void)
{
	if(manager.sampling_thread != -1)
	{
		struct itimerval stopval;

		// stop timer
		stopval.it_interval.tv_sec = 0;
		stopval.it_interval.tv_usec = 0;
		stopval.it_value.tv_sec = 0;
		stopval.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &stopval, NULL);

		pthread_kill(manager.sampling_thread, SIGUSR1);
		LOGI("join sampling thread started\n");
		pthread_join(manager.sampling_thread, NULL);
		LOGI("join sampling thread done\n");

		manager.sampling_thread = -1;
	}

	return 0;
}

static useconds_t time_diff_us(struct timeval *tv1, struct timeval *tv2)
{
	return (tv1->tv_sec - tv2->tv_sec) * 1000000 +
		((int)tv1->tv_usec - (int)tv2->tv_usec);
}

static pthread_mutex_t replay_thread_mutex = PTHREAD_MUTEX_INITIALIZER;

static void exit_replay_thread()
{
	pthread_mutex_lock(&replay_thread_mutex);
	manager.replay_thread = -1;
	reset_replay_event_seq(&prof_session.replay_event_seq);
	pthread_mutex_unlock(&replay_thread_mutex);
}

static void *replay_thread(void *arg)
{
	struct replay_event_seq_t *event_seq = (struct replay_event_seq_t *)arg;
	int i = 0;
	useconds_t ms;
	useconds_t prev_event_offset = 0;

	struct replay_event_t * pevent = NULL;

	LOGI("replay events thread started. event num = %d\n", event_seq->event_num);
	if (event_seq->event_num != 0)
	{
		pevent = event_seq->events;
	}

	for (i = 0; i < event_seq->event_num; i++) {
		useconds_t event_offset = time_diff_us(&pevent->ev.time, &event_seq->tv);
		if (event_offset >= prev_event_offset)
			ms = event_offset - prev_event_offset;
		else
			ms = 0;

#ifdef THREAD_REPLAY_DEBUG
		print_replay_event(pevent, i + 1, "\t");
#endif
		LOGI_th_rep("%d) sleep %d\n", i, ms);
		usleep(ms);

		write_input_event(pevent->id, &pevent->ev);

		prev_event_offset = event_offset;

		pevent++;
	}

	LOGI("replay events thread finished\n");

	exit_replay_thread();

	return arg;
}

int start_replay()
{
	int res = 0;

	pthread_mutex_lock(&replay_thread_mutex);

	if (manager.replay_thread != -1) {
		LOGI("replay already started\n");
		res = 1;
		goto exit;
	}

	if (pthread_create(&(manager.replay_thread),
			   NULL,
			   replay_thread,
			   &prof_session.replay_event_seq) < 0)
	{
		LOGE("Failed to create replay thread\n");
		res = 1;
		goto exit;
	}

exit:
	pthread_mutex_unlock(&replay_thread_mutex);
	return res;
}


void stop_replay()
{
	pthread_mutex_lock(&replay_thread_mutex);

	if (manager.replay_thread == -1) {
		LOGI("replay thread not running\n");
		goto exit;
	}
	LOGI("stopping replay thread\n");
	pthread_cancel(manager.replay_thread);
	pthread_join(manager.replay_thread, NULL);
	manager.replay_thread = -1;
	LOGI("replay thread joined\n");

	reset_replay_event_seq(&prof_session.replay_event_seq);

exit:
	pthread_mutex_unlock(&replay_thread_mutex);

}
