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

#define SUBBUF_SIZE 64 * 1024
#define SUBBUF_NUM 32 * 16

static int open_buf(void)
{
	manager.buf_fd = open(BUF_FILENAME, O_RDWR);
	if (manager.buf_fd == -1) {
		LOGE("Cannot open buffer: %s\n", strerror(errno));
		return 1;
	}
	LOGI("buffer opened: %s, %d\n", BUF_FILENAME, manager.buf_fd);

	return 0;
}

static void close_buf(void)
{
	LOGI("close buffer (%d)\n", manager.buf_fd);
	close(manager.buf_fd);
}

static int insert_buf_modules(void)
{
	if (system("cd /opt/swap/sdk && ./start.sh")) {
		LOGE("Cannot insert swap modules\n");
		return -1;
	}

	return 0;
}

static void remove_buf_modules(void)
{
	LOGI("rmmod buffer start\n");
	if (system("cd /opt/swap/sdk && ./stop.sh")) {
		LOGW("Cannot remove swap modules\n");
	}
	LOGI("rmmod buffer done\n");
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

	if (open_buf() != 0) {
		LOGE("Cannot open buffer\n");
		remove_buf_modules();
		return 1;
	}

	if (ioctl(manager.buf_fd, SWAP_DRIVER_BUFFER_INITIALIZE, &init) == -1) {
		LOGE("Cannot init buffer: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

void exit_buf(void)
{
	LOGI("Uninit buffer (%d)\n", manager.buf_fd);
	if (ioctl(manager.buf_fd, SWAP_DRIVER_BUFFER_UNINITIALIZE) == -1)
		LOGW("Cannot uninit buffer: %s\n", strerror(errno));

	close_buf();
	remove_buf_modules();
}

void flush_buf(void)
{
	if (ioctl(manager.buf_fd, SWAP_DRIVER_FLUSH_BUFFER) == -1)
		LOGW("Cannot flush buffer: %s\n", strerror(errno));
}

int write_to_buf(struct msg_data_t *msg)
{
	if (write(manager.buf_fd, msg, MSG_DATA_HDR_LEN + msg->len) == -1) {
		LOGE("write to buf: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

