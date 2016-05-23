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
#include "swap_debug.h"
#include "transfer_thread.h"

#define BUF_SIZE 4096

static void transfer_thread_cleanup(void *arg)
{
	int *fd_pipe;
	fd_pipe = (int *) arg;

	close(fd_pipe[0]);
	close(fd_pipe[1]);

}

static void *transfer_thread(void *arg)
{
	(void)arg;
	int fd_pipe[2];
	ssize_t nrd, nwr;

	//init thread
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	//init pipe
	if (pipe(fd_pipe) != 0) {
		/* TODO posible need total instrumentation stop up there */
		SWAP_LOGE("can not pipe!\n");
		return NULL;
	}

	//set cleanup function
	pthread_cleanup_push(transfer_thread_cleanup, (void *)fd_pipe);

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	SWAP_LOGI("transfer thread started\n");

	while (1) {
		nrd = splice(manager.buf_fd, NULL,
			     fd_pipe[1], NULL,
			     BUF_SIZE, 0);
		if (nrd == -1) {
			int errsv = errno;
			if (errsv == EAGAIN) {
				SWAP_LOGI("No more data to read\n");
				break;
			}
			GETSTRERROR(errsv, err_buf);
			SWAP_LOGE("Cannot splice read: %s\n", err_buf);
			goto thread_exit;
		}

		nwr = splice(fd_pipe[0], NULL,
			     manager.host.data_socket, NULL,
			     nrd, 0);

		if (nwr == -1) {
			GETSTRERROR(errno, buf);
			SWAP_LOGE("Cannot splice write: %s\n", buf);
			goto thread_exit;
		}
		if (nwr != nrd) {
			SWAP_LOGW("nrd - nwr = %d\n", nrd - nwr);
		}
	}

	thread_exit:
	pthread_cleanup_pop(1);

	SWAP_LOGI("transfer thread finished. return\n");

	return NULL;
}

int start_transfer(void)
{
	int saved_flags;

	if (manager.host.data_socket == -1) {
		SWAP_LOGW("won't start transfer thread: data socket isn't open\n");
		return 0;
	}

	if (manager.transfer_thread != -1) { // already started
		SWAP_LOGW("transfer already running\n");
		stop_transfer();
	}

	saved_flags = fcntl(manager.buf_fd, F_GETFL);
	if (fcntl(manager.buf_fd, F_SETFL, saved_flags & ~O_NONBLOCK) == -1) {
		SWAP_LOGE("can not set buf_fd flags\n");
		return -1;
	}

	if(pthread_create(&(manager.transfer_thread),
			  NULL,
			  transfer_thread,
			  NULL) < 0)
	{
		SWAP_LOGE("Failed to create transfer thread\n");
		return -1;
	}

	return 0;
}

void stop_transfer(void)
{
	int saved_flags;
	int ret = 0;

	if (manager.transfer_thread == -1) {
		SWAP_LOGI("transfer thread not running\n");
		return;
	}
	SWAP_LOGI("stopping transfer\n");

	flush_buf();
	saved_flags = fcntl(manager.buf_fd, F_GETFL);
	if (fcntl(manager.buf_fd, F_SETFL, saved_flags | O_NONBLOCK) == -1) {
		/* TODO do something on error */
		SWAP_LOGE("can not set buf_fd flags\n");
	}
	wake_up_buf();

	SWAP_LOGI("joining thread...\n");
	ret = pthread_join(manager.transfer_thread, NULL);
	if (ret != 0)
		SWAP_LOGW("pthread_join: unknown error %d\n", ret);

	manager.transfer_thread = -1;

	SWAP_LOGI("transfer thread stoped\n");
}
