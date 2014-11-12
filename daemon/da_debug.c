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
#include "debug.h"
#include <errno.h>

#define DEBUG_LOGFILE		"/tmp/daemonlog.da"

#if DEBUG
static inline int close_on_exec_dup(int old, int new)
{
	int ret = -1;

	if (dup2(old, new) != -1) {
		unsigned long flags = fcntl(new, F_GETFD);
		if (flags == -1) {
			LOGE("can not get flags fd #%d <%s>\n", new,
			     strerror(errno));
			goto err_ret;
		}

		if (fcntl(new, F_SETFD, flags | FD_CLOEXEC) == -1) {
			LOGE("can not get flags fd #%d <%s>\n", new,
			     strerror(errno));
			goto err_ret;
		}
	} else {
		LOGE("dup2 fail\n");
		goto err_ret;
	}

	/* success */
	ret = 0;

err_ret:
	return ret;
}

int initialize_log(void)
{
	/* TODO fix problem with ecore and redirect stderr to DEBUG_LOGFILE back
	 *
	 * error sample
	 * *** IN FUNCTION: _ecore_main_fdh_epoll_mark_active()
	 * ERR<2328>:ecore ecore.c:572 _ecore_magic_fail()   Input handle has already been freed!
	 * ERR<2328>:ecore ecore.c:581 _ecore_magic_fail() *** NAUGHTY PROGRAMMER!!!
	 * *** SPANK SPANK SPANK!!!
	 *
	 */
	int ret = 0;
	int fd = -1;
	int fd_null = -1;

	if (remove(DEBUG_LOGFILE))
		LOGE("remove(%s), return error, errno=%d\n",
		     DEBUG_LOGFILE, errno);

	fd = open(DEBUG_LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	fd_null = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0777);

	if (fd != -1 && fd_null != -1) {
		if (close_on_exec_dup(fd_null, 1) != 0 ||
		    close_on_exec_dup(fd, 2) != 0) {
			LOGE("duplicate fd fail\n");
			ret = -1;
		}
	} else {
		close(1);
		close(2);
	}

	if (fd_null != -1)
		close(fd_null);

	if (fd != -1)
		close(fd);

	close(0);
	return ret;
}

#else
int initialize_log(void)
{
	return 0;
}
#endif
