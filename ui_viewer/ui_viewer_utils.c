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
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <sys/socket.h>

#include <Elementary.h>

#include "ui_viewer_lib.h"
#include "ui_viewer_utils.h"
#include "ui_viewer_data.h"

struct temp_file_t {
	FILE *file;
	char *name;
};

static const ssize_t TMP_BUF_SIZE = 262144;
static const char log_filename[] = "/tmp/uilib.log";
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static pid_t gPid = -1;
static __thread pid_t		gTid = -1;

extern __traceInfo gTraceInfo;

void reset_pid_tid()
{
	gPid = -1;
	gTid = -1;
}

// return current process id
pid_t _getpid()
{
	if (gPid == -1)
		gPid = getpid();
	return gPid;
}

// return current thread id
pid_t _gettid()
{
	if(gTid == -1)
		gTid = syscall(__NR_gettid);	// syscall is very expensive
	return gTid;
}

char * _strncpy(char *dest, const char *src, size_t n)
{
	if (!src) {
		if (dest)
			dest[0] = '\0';
	} else {
		strncpy(dest, src, n);
		if (n > 0)
			dest[n - 1] = '\0';
	}

	return dest;
}

bool printLog(log_t *log, int msgType)
{
	ssize_t res, len;
	if (gTraceInfo.socket.daemonSock == -1)
		return false;

	if (log == NULL)
		return false;

	log->type = msgType;
	len = sizeof(log->type) + sizeof(log->length) + log->length;

	pthread_mutex_lock(&(gTraceInfo.socket.sockMutex));
	res = send(gTraceInfo.socket.daemonSock, log, len, 0);
	pthread_mutex_unlock(&(gTraceInfo.socket.sockMutex));

	return (res == len);
}

bool print_log_str(int msgType, char *str)
{
	log_t log;
	ssize_t res, len;

	if (gTraceInfo.socket.daemonSock == -1)
		return false;

	log.type = msgType;
	if (str != NULL)
		log.length = snprintf(log.data, sizeof(log.data), str);
	else
		log.length = 0;

	len = sizeof(log.type) + sizeof(log.length) + log.length;

	pthread_mutex_lock(&(gTraceInfo.socket.sockMutex));
	res = send(gTraceInfo.socket.daemonSock, &log, len, 0);
	pthread_mutex_unlock(&(gTraceInfo.socket.sockMutex));

	return (res == len);
}

bool print_log_fmt(int msgType, const char *func_name, int line, ...)
{
	ssize_t res = 0, len = 0;
	char *fmt, *p;
	int n;
	log_t log;
	va_list ap;

	log.type = msgType;

	p = log.data;
	n = snprintf(p, sizeof(log.data), "[%05d:%05d]%s:%d)", _getpid(),
		     _gettid(),  func_name, line);
	p += n;

	va_start(ap, line);
	fmt = va_arg(ap, char *);
	if (fmt != NULL) {
		if (strchr(fmt, '%') == NULL)
			n += snprintf(p, sizeof(log.data) - n, "%s", fmt);
		else
			n += vsnprintf(p, sizeof(log.data) - n, fmt, ap);
	}
	va_end(ap);

	if (n > -1 && n < (int)sizeof(log.data)) {
		log.length = n;
	} else {
		ui_viewer_log("Log pack error\n");
		log.length = 0;
	}

	len = sizeof(log.type) + sizeof(log.length) + log.length;

	pthread_mutex_lock(&(gTraceInfo.socket.sockMutex));

	if (gTraceInfo.socket.daemonSock != -1) {
		res = send(gTraceInfo.socket.daemonSock, &log, len, MSG_DONTWAIT);
	} else {
		ui_viewer_log("%d %s\n", msgType, log.data);
		fflush(stderr);
	}

	pthread_mutex_unlock(&(gTraceInfo.socket.sockMutex));

	return (res == len);
}

static int tmp_file_open(struct temp_file_t *tmp_file)
{
	char template_name[] = TMP_DIR"/swap_ui_viewer_XXXXXX";
	FILE *file;

	mktemp(template_name);
	file = fopen(template_name, "w");
	if (file == NULL)
		return -1;

	if (tmp_file->name != NULL)
		free(tmp_file->name);

	tmp_file->name = malloc(strlen(template_name) + 1);
	if (tmp_file->name == NULL)
		goto tmp_file_open_no_mem;

	strcpy(tmp_file->name, template_name);

	tmp_file->file = file;

	return 0;

tmp_file_open_no_mem:
	fclose(file);

	return -ENOMEM;
}

static void tmp_file_close(struct temp_file_t *tmp_file)
{
	free(tmp_file->name);
	fclose(tmp_file->file);
}


bool print_log_ui_viewer_hierarchy_status(enum ErrorCode *err_code)
{
	log_t log;
	ssize_t res, len;
	char *log_ptr;

	*err_code = ERR_NO;

	if (gTraceInfo.socket.daemonSock == -1)
		return false;

	// check if hierarchy request is proceeding now
	if (get_hierarchy_status() == HIERARCHY_RUNNING)
		*err_code = ERR_ALREADY_RUNNING;

	log.type = APP_MSG_GET_UI_HIERARCHY;
	log_ptr = pack_int32(log.data, (uint32_t)*err_code);
	log.length = log_ptr - log.data;
	len = sizeof(log.type) + sizeof(log.length) + log.length;

	pthread_mutex_lock(&(gTraceInfo.socket.sockMutex));
	res = send(gTraceInfo.socket.daemonSock, &log, len, 0);
	pthread_mutex_unlock(&(gTraceInfo.socket.sockMutex));

	return (res == len);
}

bool print_log_ui_viewer_hierarchy_error(void)
{
	log_t log;
	ssize_t res, len;
	char *log_ptr;
	enum ErrorCode err_code;

	// TODO: system error
	err_code = ERR_UNKNOWN;

	if (gTraceInfo.socket.daemonSock == -1)
		return false;

	log.type = APP_MSG_GET_UI_HIERARCHY_DATA;
	log_ptr = pack_int32(log.data, (uint32_t)err_code);
	log.length = log_ptr - log.data;
	len = sizeof(log.type) + sizeof(log.length) + log.length;

	pthread_mutex_lock(&(gTraceInfo.socket.sockMutex));
	res = send(gTraceInfo.socket.daemonSock, &log, len, 0);
	pthread_mutex_unlock(&(gTraceInfo.socket.sockMutex));

	return (res == len);
}

void* print_log_ui_viewer_info_list(void *prendering)
{
	log_t log;
	ssize_t res, len;
	char *log_ptr, *tmp_ptr;
	struct temp_file_t tmp_f;
	struct timeval start_tv, finish_tv, tv;
	Eina_Bool rendering, cancelled = EINA_FALSE;

	if (gTraceInfo.socket.daemonSock == -1)
		return NULL;

	if (get_hierarchy_status() == HIERARCHY_RUNNING)
		return NULL;

	set_hierarchy_status(HIERARCHY_RUNNING);

	rendering = *(Eina_Bool*)prendering;

	if (tmp_file_open(&tmp_f) != 0)
		return NULL;

	gettimeofday(&start_tv, NULL);
	pack_ui_obj_info_list(tmp_f.file, rendering, &cancelled);
	gettimeofday(&finish_tv, NULL);
	timersub(&finish_tv, &start_tv, &tv);
	ui_viewer_log("getting hierarchy time : %d sec, %d usec\n",
		      tv.tv_sec, tv.tv_usec);

	if (!cancelled) {
		log.type = APP_MSG_GET_UI_HIERARCHY_DATA;
		log_ptr = pack_string(log.data, tmp_f.name);
		log.length = log_ptr - log.data;
		len = sizeof(log.type) + sizeof(log.length) + log.length;

		pthread_mutex_lock(&(gTraceInfo.socket.sockMutex));
		res = send(gTraceInfo.socket.daemonSock, &log, len, 0);
		pthread_mutex_unlock(&(gTraceInfo.socket.sockMutex));

		ui_viewer_log("getting hierarchy filename: %s\n",
			      log.data);

		if (res != len)
			ui_viewer_log("can't send hierarchy info\n");
		set_hierarchy_status(HIERARCHY_NOT_RUNNING);
	}

	tmp_file_close(&tmp_f);

	return NULL;
}

bool print_log_ui_obj_screenshot(Evas_Object *obj)
{
	log_t log;
	ssize_t res, len;
	char *log_ptr;

	if (gTraceInfo.socket.daemonSock == -1)
		return false;

	log.type = APP_MSG_GET_UI_SCREENSHOT;
	log_ptr = pack_ui_obj_screenshot(log.data, obj);
	log.length = log_ptr - log.data;
	len = sizeof(log.type) + sizeof(log.length) + log.length;

	pthread_mutex_lock(&(gTraceInfo.socket.sockMutex));
	res = send(gTraceInfo.socket.daemonSock, &log, len, 0);
	pthread_mutex_unlock(&(gTraceInfo.socket.sockMutex));

	return (res == len);
}

void raise_app_window(void)
{
	x_raise_win(gPid);
}

void ui_viewer_clean_log(void)
{
	remove(log_filename);
}

void ui_viewer_log(const char *format, ...)
{
	FILE *fp;
	va_list args;

	pthread_mutex_lock(&log_lock);
	fp = fopen(log_filename, "a");
	if (fp != NULL) {
		va_start (args, format);
		vfprintf (fp, format, args);
		va_end (args);
		fclose(fp);
	}
	pthread_mutex_unlock(&log_lock);
}
