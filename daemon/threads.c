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

#define TIMER_INTERVAL_SEC			1
#define TIMER_INTERVAL_USEC			0

static void* recvThread(void* data)
{
	int index = (int)data;
	int pass = 0;
	uint64_t event;
	ssize_t recvLen;
	msg_t log;

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

		// send to host
		if (likely(log.length > 0))
		{
			recvLen = recv(manager.target[index].socket, log.data, log.length, MSG_WAITALL);
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
					// MSG_PID parsing error
					// send error message to host
					log.type = MSG_ERROR;
					log.length = sprintf(log.data, "Process id information of target application is not correct.");
					sendDataToHost(&log);

					// send stop message to main thread
					event = EVENT_STOP;
					write(manager.target[index].event_fd, &event, sizeof(uint64_t));
					break;
				}
				barloc[0] = '\0';
				barloc++;

				manager.target[index].pid = atoi(log.data);
				manager.target[index].starttime = str_to_uint64(barloc);

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
			break;
		}
		else if(log.type == MSG_MSG)
		{
			// don't send to host
			LOGI("EXTRA MSG %d|%d|%s\n", log.type, log.length, log.data);
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

		if(manager.target[index].pid != -1)
			sendDataToHost(&log);
	}

	manager.target[index].recv_thread = -1;
	return NULL;
}

int makeRecvThread(int index)
{
	if(manager.target[index].socket == -1)
		return -1;

	if(pthread_create(&(manager.target[index].recv_thread), NULL, recvThread, (void*)index) < 0)
	{
		LOGE("Failed to create recv thread for socket (%d)\n", manager.target[index].socket);
		return -1;
	}

	return 0;
}

//static 
void* samplingThread(void* data)
{
	int err, signo, i;
	uint32_t res;
	int pidarr[MAX_TARGET_COUNT];
	int pidcount;
	sigset_t waitsigmask;
	struct msg_data_t log;

	LOGI("sampling thread started\n");

	sigemptyset(&waitsigmask);
	sigaddset(&waitsigmask, SIGALRM);
	sigaddset(&waitsigmask, SIGUSR1);

	while (1) {
		err = sigwait(&waitsigmask, &signo);
		if(err != 0)
		{
			LOGE("Failed to sigwait() in sampling thread\n");
			continue;
		}

		if(signo == SIGALRM)
		{
			pidcount = 0;
			for(i = 0; i < MAX_TARGET_COUNT; i++)
			{
				if(manager.target[i].socket != -1 && manager.target[i].pid != -1)
					pidarr[pidcount++] = manager.target[i].pid;
			}

//#ifndef HOST_BUILD
			struct system_info_t sys_info;
			if (get_system_info(&sys_info) == -1) {
				LOGE("Cannot get system info\n");
			}

			struct msg_t *msg;
			msg = pack_system_info(&sys_info);
			if (!msg) {
				LOGE("Cannot pack system info\n");
			}
			if (write_to_buf(msg) == -1) {
				LOGE("Cannot write system info to buffer\n");
			}

			free_msg_data(msg);
			free_sys_info(&sys_info);
			//res = get_resource_info_new(&log.payload, DA_MSG_MAX, pidarr, pidcount);
			/* res = gen_message_sytem_info(&log, DA_MSG_MAX, pidarr, pidcount); */
			/* if(res > 0) */
			/* { */
			/* 	LOGI("payload_len=%d\n",log.len); */
			/* 	LOGI("sizeof(float) = %d\n", sizeof(float)); */
			/* 	//sendDataToHost(&log); */
			/* 	pseudoSendDataToHost(&log); */
			/* } */
			/* break; */
//#endif
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
int samplingStart()
{
	struct itimerval timerval;

	if(manager.sampling_thread != -1)	// already started
		return 1;

	if(pthread_create(&(manager.sampling_thread), NULL, samplingThread, NULL) < 0)
	{
		LOGE("Failed to create sampling thread\n");
		return -1;
	}

	timerval.it_interval.tv_sec = TIMER_INTERVAL_SEC;
	timerval.it_interval.tv_usec = TIMER_INTERVAL_USEC;
	timerval.it_value.tv_sec = TIMER_INTERVAL_SEC;
	timerval.it_value.tv_usec = TIMER_INTERVAL_USEC;
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
