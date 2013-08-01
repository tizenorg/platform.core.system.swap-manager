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

#include "daemon.h"
#include "utils.h"
#include "sys_stat.h"

#include "da_protocol.h"
#include "da_data.h"
#include "debug.h"
#include "process_info.h"

//#define DEBUG_GSI

static void* recvThread(void* data)
{
	int index = (int)data;
	int pass = 0;
	uint64_t event;
	ssize_t recvLen;
	msg_target_t log;

	// initialize target variable
	manager.target[index].pid = -1;
	manager.target[index].starttime = 0;
	manager.target[index].allocmem = 0;

	while(1)
	{
		// read from target process
		recvLen = recv(manager.target[index].socket, &log,
				sizeof(log.type) + sizeof(log.length), MSG_WAITALL);

		if(unlikely(recvLen < sizeof(log.type) + sizeof(log.length)))
		{	// disconnect
			event = EVENT_STOP;
			write(manager.target[index].event_fd, &event, sizeof(uint64_t));
			break;
		}
		if (IS_PROBE_MSG(log.type)) {
			struct msg_data_t tmp_msg;
			int offs = sizeof(log.type) + sizeof(log.length);
			recvLen = recv(manager.target[index].socket,
				       (char *)&log + offs,
				       MSG_DATA_HDR_LEN - offs,
				       MSG_WAITALL);
			memcpy(&tmp_msg, &log, MSG_DATA_HDR_LEN);
			struct msg_data_t *msg = malloc(MSG_DATA_HDR_LEN +
							tmp_msg.len);
			memcpy(msg, &tmp_msg, MSG_DATA_HDR_LEN);
			recvLen = recv(manager.target[index].socket,
				       (char *)msg + MSG_DATA_HDR_LEN,
				       msg->len, MSG_WAITALL);
			write_to_buf(msg);
			free(msg);
			continue;
		}

		// send to host
		if (likely(log.length > 0))
		{
			recvLen = recv(manager.target[index].socket,
							log.data, log.length, MSG_WAITALL);
			if(unlikely(recvLen != log.length))	// consume as disconnect
			{
				event = EVENT_STOP;
				write(manager.target[index].event_fd, &event, sizeof(uint64_t));
				break;
			}
		}

		log.data[log.length] = '\0';
		if(log.type == MSG_ALLOC)
		{
			manager.target[index].allocmem = str_to_int64(log.data);
			continue;		// don't send to host
		}
		else if(log.type == MSG_PID)
		{
			LOGI("MSG_PID arrived : %s\n", log.data);

			// only when first MSG_PID is arrived
			if(manager.target[index].pid == -1)
			{
				char* barloc;
				barloc = strchr(log.data, '|');
				if(barloc == NULL)
				{
					// TODO: complain to host about wrong pid message
					// send stop message to main thread
					event = EVENT_STOP;
					write(manager.target[index].event_fd, &event, sizeof(uint64_t));
					break;
				}
				barloc[0] = '\0';
				barloc++;

				manager.target[index].pid = atoi(log.data);
				manager.target[index].starttime = str_to_uint64(barloc);

				write_process_info(manager.target[index].pid,
						   manager.target[index].starttime);

				event = EVENT_PID;
				write(manager.target[index].event_fd, &event, sizeof(uint64_t));
			}
			continue;		// don't send to host
		}
		else if(log.type == MSG_TERMINATE)
		{
			// send stop message to main thread
			event = EVENT_STOP;
			write(manager.target[index].event_fd, &event, sizeof(uint64_t));

			struct timeval tv;
			gettimeofday(&tv, NULL);
			struct msg_data_t msg = {
				.id = NMSG_TERMINATE,
				.seq_num = 0,
				.sec = tv.tv_sec,
				.nsec = tv.tv_usec * 1000,
				.len = 0
			};
			write_to_buf(&msg);
			break;
		}
		else if(log.type == MSG_MSG)
		{
			// don't send to host
			LOGI("EXTRA MSG type=%d;len=%d;data='%s'\n", log.type, log.length, log.data);
			continue;
		}
#ifdef PRINT_TARGET_LOG
		else if(log.type == MSG_LOG)
		{
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
		}
		else if(log.type == MSG_IMAGE)
		{
			LOGI("MSG_IMAGE received\n");
		}
		else 	// not MSG_LOG and not MSG_IMAGE
		{
			LOGI("Extra MSG TYPE (%d|%d|%s)\n", log.type, log.length, log.data);
		}
#endif

		// do not send any message to host until MSG_PID message arrives
		if(unlikely(pass == 0))
		{
			while(manager.target[index].initial_log == 0)
			{
				sleep(0);
			}
		}
		pass = 1;
	}

	manager.target[index].recv_thread = -1;
	return NULL;
}

int makeRecvThread(int index)
{
	if (manager.target[index].socket == -1)
		return -1;

	if (pthread_create(&(manager.target[index].recv_thread),
		NULL, recvThread, (void*)index) < 0)
	{
		LOGE("Failed to create recv thread for socket (%d)\n",
				manager.target[index].socket);
		return -1;
	}

	return 0;
}

//static
void* samplingThread(void* data)
{
	int err, signo, i;
	//debug only
#ifdef DEBUG_GSI
	int pidarr[1] = {656};//{3619}; //DEBUG ONLY
	int pidcount = 1;
#else
	int pidarr[MAX_TARGET_COUNT];
	int pidcount;
#endif
	sigset_t waitsigmask;

	LOGI("sampling thread started\n");

	sigemptyset(&waitsigmask);
	sigaddset(&waitsigmask, SIGALRM);
	sigaddset(&waitsigmask, SIGUSR1);

	while (1) {
		//debug only
#ifdef DEBUG_GSI
		err = 0;
#else
		err = sigwait(&waitsigmask, &signo);
#endif
		if(err != 0)
		{
			LOGE("Failed to sigwait() in sampling thread\n");
			continue;
		}

		//debug only
#ifdef DEBUG_GSI
		if(1)
#else
		if(signo == SIGALRM)
#endif
		{


#ifndef DEBUG_GSI
			pidcount = 0;
			for(i = 0; i < MAX_TARGET_COUNT; i++)
			{
				//LOGI("#%d: sock=%d; pid=%d;\n", i,
				//		manager.target[i].socket , manager.target[i].pid );
				if(manager.target[i].socket != -1 && manager.target[i].pid != -1)
					pidarr[pidcount++] = manager.target[i].pid;
			}
#endif
			struct system_info_t sys_info;
			if (get_system_info(&sys_info, pidarr, pidcount) == -1) {
				LOGE("Cannot get system info\n");
				//do not send sys_info because
				//it is corrupted
				continue;
			}

			struct msg_data_t *msg;
			msg = pack_system_info(&sys_info);
			if (!msg) {
				LOGE("Cannot pack system info\n");
				reset_system_info(&sys_info);
				continue;
			}

			write_to_buf(msg);

			printBuf((char *)msg, MSG_DATA_HDR_LEN + msg->len);

			free_msg_data(msg);
			reset_system_info(&sys_info);
#ifdef DEBUG_GSI
			break; //FOR DEBUG ONLY
#endif

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

	close_system_file_descriptors();
	LOGI("sampling thread ended\n");
	return NULL;
}

// return 0 if normal case
// return minus value if critical error
// return plus value if non-critical error
int samplingStart()
{
	struct itimerval timerval;
	time_t sec = prof_session.conf.system_trace_period / 1000;
	suseconds_t usec = prof_session.conf.system_trace_period * 1000 %
		1000000;

	if(manager.sampling_thread != -1)	// already started
		return 1;

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

int samplingStop()
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
		pthread_join(manager.sampling_thread, NULL);

		manager.sampling_thread = -1;
	}

	return 0;
}

static useconds_t time_diff_us(struct timeval *tv1, struct timeval *tv2)
{
	return (tv1->tv_sec - tv2->tv_sec) * 1000000 +
		((int)tv1->tv_usec - (int)tv2->tv_usec);
}

static void *replay_thread(void *arg)
{
	struct replay_event_seq_t *event_seq = (struct replay_event_seq_t *)arg;
	int i = 0;
	useconds_t ms;
	useconds_t prev_event_offset = 0;

	struct replay_event_t * pevent = NULL;

	LOGI_th_rep("replay events thread started\n");
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

		/* filter touch and key events here
		   and process them separately */
		switch (pevent->id)
		{
		case INPUT_ID_TOUCH:
			LOGI_th_rep("event -> %s\n", INPUT_ID_STR_KEY);
			_device_write(g_touch_dev, &pevent->ev);
			break;

		case INPUT_ID_KEY:
			LOGI_th_rep("event -> %s\n", INPUT_ID_STR_TOUCH);
			_device_write(g_key_dev, &pevent->ev);
			break;
		default:
			LOGE("event -> UNKNOWN INPUT ID");
		}

		prev_event_offset = event_offset;

		pevent++;
	}

	LOGI("replay events thread finished\n");

	return arg;
}

int start_replay()
{
	if (manager.replay_thread != -1) // already started
		return 1;

	if (pthread_create(&(manager.replay_thread),
			   NULL,
			   replay_thread,
			   &prof_session.replay_event_seq) < 0)
	{
		LOGE("Failed to create replay thread\n");
		return 1;
	}

	return 0;
}

void stop_replay()
{
	if (manager.replay_thread == -1) {
		LOGI("replay thread not running\n");
		return;
	}
	LOGI("stopping replay thread\n");
	pthread_cancel(manager.replay_thread);
	pthread_join(manager.replay_thread, NULL);
	manager.replay_thread = -1;
	reset_replay_event_seq(&prof_session.replay_event_seq);
	LOGI("replay thread joined\n");
}
