/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
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
 * - Samsung RnD Institute Russia
 *
 */


#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "daemon.h"
#include "buffer.h"
#include "debug.h"
#include "transfer_thread.h"

#define BUF_SIZE 4096

static void *transfer_thread(void *arg)
{
	(void)arg;
	int fd_pipe[2];
	ssize_t nrd, nwr;

	LOGI("transfer thread started\n");

	pipe(fd_pipe);
	while (1) {
		nrd = splice(manager.buf_fd, NULL,
			     fd_pipe[1], NULL,
			     BUF_SIZE, 0);
		if (nrd == -1) {
			if (errno == EAGAIN) {
				LOGI("No more data to read\n");
				break;
			}
			LOGE("Cannot splice read: %s\n", strerror(errno));
			goto thread_exit;
		}

		nwr = splice(fd_pipe[0], NULL,
			     manager.host.data_socket, NULL,
			     nrd, 0);
		if (nwr == -1) {
			LOGE("Cannot splice write: %s\n", strerror(errno));
			goto thread_exit;
		}
		if (nwr != nrd) {
			LOGW("nrd - nwr = %d\n", nrd - nwr);
		}
	}

	thread_exit:

	close(fd_pipe[0]);
	close(fd_pipe[1]);

	LOGI("transfer thread finished\n");

	return NULL;
}

int start_transfer()
{
	int saved_flags;

	if (manager.host.data_socket == -1) {
		LOGW("won't start transfer thread: data socket isn't open\n");
		return 0;
	}

	if (manager.transfer_thread != -1) { // already started
		LOGW("transfer already running\n");
		stop_transfer();
	}

	saved_flags = fcntl(manager.buf_fd, F_GETFL);
	fcntl(manager.buf_fd, F_SETFL, saved_flags & ~O_NONBLOCK);

	if(pthread_create(&(manager.transfer_thread),
			  NULL,
			  transfer_thread,
			  NULL) < 0)
	{
		LOGE("Failed to create transfer thread\n");
		return -1;
	}

	return 0;
}

#define SECS_TO_FORCE_STOP 10
void stop_transfer()
{
	int saved_flags;
	int ret = 0;
	struct timeval tv;
	struct timespec ts;

	if (manager.transfer_thread == -1) {
		LOGI("transfer thread not running\n");
		return;
	}
	LOGI("stopping transfer\n");

	flush_buf();
	saved_flags = fcntl(manager.buf_fd, F_GETFL);
	fcntl(manager.buf_fd, F_SETFL, saved_flags | O_NONBLOCK);

	gettimeofday(&tv, NULL);
	ts.tv_sec = tv.tv_sec + SECS_TO_FORCE_STOP;
	ts.tv_nsec = tv.tv_usec * 1000;
	ret = pthread_timedjoin_np(manager.transfer_thread, NULL, &ts);
	if (ret == ETIMEDOUT) {
		LOGW("transfer thread is not joined in %d sec, force stop\n",
			SECS_TO_FORCE_STOP);
		ret = pthread_cancel(manager.transfer_thread);
		if (ret == ESRCH)
			LOGW("no transfer thread to force stop\n");
	} else if (ret != 0)
		LOGW("pthread_timedjoin_np: unknown error %d\n", ret);

	manager.transfer_thread = -1;

	LOGI("transfer thread stoped\n");
}
