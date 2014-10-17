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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "daemon.h"
#include "buffer.h"
#include "ioctl_commands.h"
#include "debug.h"

#define SUBBUF_SIZE 32 * 1024
#define SUBBUF_NUM 8

static int open_tasks_dev(void)
{
	if (manager.fd.inst_tasks == NULL) {
		manager.fd.inst_tasks = fopen(INST_PID_FILENAME, "re");
		if (manager.fd.inst_tasks == NULL) {
			LOGE("Cannot open tasks dev: %s\n", strerror(errno));
			return 1;
		}
		LOGI("tasks dev opened: %s, %d\n", BUF_FILENAME, manager.buf_fd);
	} else {
		LOGW("tasks dev double open try: <%s>\n", INST_PID_FILENAME);
	}

	return 0;
}

static void close_tasks_dev(void)
{
	LOGI("close tasks dev (%d)\n", manager.fd.inst_tasks);
	if (manager.fd.inst_tasks != NULL)
		fclose(manager.fd.inst_tasks);
	else
		LOGW("manager.fd.inst_tasks alredy closed or not opened\n");
}

static int open_buf_ctl(void)
{
	manager.buf_fd = open(BUF_FILENAME, O_RDONLY | O_CLOEXEC);
	if (manager.buf_fd == -1) {
		LOGE("Cannot open buffer: %s\n", strerror(errno));
		return 1;
	}
	LOGI("buffer opened: %s, %d\n", BUF_FILENAME, manager.buf_fd);

	manager.user_ev_fd = open(USER_EVENT_FILENAME, O_WRONLY | O_CLOEXEC);
	if (manager.user_ev_fd == -1) {
		LOGE("Cannot open user event sysfs file: %s\b", strerror(errno));
		return 1;
	}
	LOGI("user event sysfs file opened: %s, %d\n", USER_EVENT_FILENAME,
	     manager.user_ev_fd);

	return 0;
}

static void close_buf_ctl(void)
{
	LOGI("close buffer (%d)\n", manager.buf_fd);
	close(manager.buf_fd);

	LOGI("close user event sysfs file (%d)\n", manager.user_ev_fd);
	close(manager.user_ev_fd);
}

static int insert_buf_modules(void)
{
	if (system("cd /opt/swap/sdk && ./start.sh")) {
		LOGE("Cannot insert swap modules\n");
		return -1;
	}

	return 0;
}

int init_buf(void)
{
	struct buffer_initialize init = {
		.size = SUBBUF_SIZE,
		.count = SUBBUF_NUM,
	};

	if (insert_buf_modules() != 0) {
		LOGE("Cannot insert buffer modules\n");
		return 1;
	}

	if (open_buf_ctl() != 0) {
		LOGE("Cannot open buffer\n");
		return 1;
	}

	if (ioctl(manager.buf_fd, SWAP_DRIVER_BUFFER_INITIALIZE, &init) == -1) {
		LOGE("Cannot init buffer: %s\n", strerror(errno));
		return 1;
	}

	if (open_tasks_dev() != 0) {
		LOGE("Cannot open tasks: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

void exit_buf(void)
{
	LOGI("Uninit driver (%d)\n", manager.buf_fd);
	if (ioctl(manager.buf_fd, SWAP_DRIVER_BUFFER_UNINITIALIZE) == -1)
		LOGW("Cannot uninit driver: %s\n", strerror(errno));

	close_buf_ctl();
	close_tasks_dev();
}

void flush_buf(void)
{
	if (ioctl(manager.buf_fd, SWAP_DRIVER_FLUSH_BUFFER) == -1)
		LOGW("Cannot send flush to driver: %s\n", strerror(errno));
}

void wake_up_buf(void)
{
	if (ioctl(manager.buf_fd, SWAP_DRIVER_WAKE_UP) == -1)
		LOGW("Cannot send wake up to driver: %s\n", strerror(errno));
}

int write_to_buf(struct msg_data_t *msg)
{
	uint32_t total_len = MSG_DATA_HDR_LEN + msg->len;

	if (write(manager.user_ev_fd, msg, total_len) == -1) {
		LOGE("write to buf (user_ev_fd=%d, msg=%p, len=%d) %s\n",
		     manager.user_ev_fd, msg, total_len, strerror(errno));
		return 1;
	}
	return 0;
}
