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
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "daemon.h"

#define DEBUG_LOGFILE		"/tmp/daemonlog.da"

#if DEBUG
static inline void close_on_exec_dup(int old, int new)
{
	dup2(old, new);
	fcntl(new, F_SETFD, fcntl(new, F_GETFD) | FD_CLOEXEC);
}

void initialize_log(void)
{
	int fd = open(DEBUG_LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd > 0) {
		close_on_exec_dup(fd, 1);
		close_on_exec_dup(fd, 2);

		close(fd);
	} else {
		close(1);
		close(2);
	}

	close(0);
}

#else
void initialize_log(void)
{
}
#endif
