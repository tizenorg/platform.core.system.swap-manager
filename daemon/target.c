/*
 *  DA manager
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Vyacheslav Cherkashin <v.cherkashin@samsung.com>
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


#define _GNU_SOURCE	/* for accept4() */
#include <sys/socket.h>

#include "target.h"

#include "thread.h"
#include "daemon.h"	// for manager (it is need delete)
#include "smack.h"
#include "swap_debug.h"


static struct target *target_malloc(void);
static void target_free(struct target *t);

struct target *target_ctor(void)
{
	struct target *t;

	t = target_malloc();
	if (t) {
		t->app_type = APP_TYPE_UNKNOWN;
		t->pid = UNKNOWN_PID;
		t->socket = UNKNOWN_FD;
		t->event_fd = UNKNOWN_FD;
		t->initial_log = 0;
		t->event_fd_released = 0;
		t->allocmem = 0;

		t->thread = thread_ctor();
		if (t->thread == NULL) {
			target_free(t);
			t = NULL;
		}
	}

	return t;
}

void target_clean(struct target *t)
{
	t->allocmem = 0;
	t->event_fd_released = 0;
	t->initial_log = 0;

	if (t->event_fd != -1)
		close(t->event_fd);
	t->event_fd = -1;

	if (t->socket != UNKNOWN_FD)
		close(t->socket);
	t->socket = -1;
}

void target_dtor(struct target *t)
{
	target_clean(t);
	thread_dtor(t->thread);
	target_free(t);
}


int target_accept(struct target *t, int sockfd)
{
	int sock;

	sock = accept4(sockfd, NULL, NULL, SOCK_CLOEXEC);
	if (sock == UNKNOWN_FD)
		return 1;

	/* accept succeed */
	fd_setup_attributes(sock);

	t->socket = sock;

	return 0;
}

int target_send_msg(struct target *t, struct msg_target_t *msg)
{
	return send_msg_to_sock(t->socket, msg);
}

int target_recv_msg(struct target *t, struct msg_target_t *msg)
{
	return recv_msg_from_sock(t->socket, msg);
}

int target_start(struct target *t, void *(*start_routine) (void *))
{
	return thread_start(t->thread, start_routine, (void *)t);
}

int target_wait(struct target *t)
{
	return thread_wait(t->thread);
}


pid_t target_get_pid(struct target *t)
{
	return t->pid;
}

void target_set_pid(struct target *t, pid_t pid)
{
	t->pid = pid;
}

pid_t target_get_ppid(struct target *t)
{
	return t->ppid;
}

void target_set_ppid(struct target *t, pid_t ppid)
{
	t->ppid = ppid;
}

static int target_cnt = 0;
static pthread_mutex_t ts_mutex = PTHREAD_MUTEX_INITIALIZER;
static int target_use[MAX_TARGET_COUNT] = {0};
static struct target target_array[MAX_TARGET_COUNT];

static void target_array_lock(void)
{
	pthread_mutex_lock(&ts_mutex);
}

static void target_array_unlock(void)
{
	pthread_mutex_unlock(&ts_mutex);
}

static struct target *target_malloc(void)
{
	int i;
	struct target *t = NULL;

	target_array_lock();
	for (i = 0; i < MAX_TARGET_COUNT; i++) {
		/* FIXME refactor this code: convert targer_use[] and
		 * event_fd_released to one counter variable */

		/* if target_use is 1 and event_fd_released 1 targed not used
		 * and can be released. target_use and event_fd_released are set
		 * in different threads asynchronously
		 */
		if (target_use[i] == 1 &&
		    target_array[i].event_fd_released == 1) {
			target_clean(&target_array[i]);
			thread_dtor(target_array[i].thread);
			if (target_use[i] == 0)
				LOGE("double free t=%p\n", &target_array[i]);
			target_use[i] = 0;
		}
	}

	for (i = 0; i < MAX_TARGET_COUNT; i++) {
		if (target_use[i] == 0) {
			target_use[i] = 1;
			t = &target_array[i];
			break;
		}
	}
	target_array_unlock();

	return t;
}

static void target_free(struct target *t)
{
	int id = t - target_array;

	target_array_lock();
	if (target_use[id] == 0)
		LOGE("double free t=%p\n", t);
	target_use[id] = 0;
	target_array_unlock();
}


void target_cnt_set(int cnt)
{
	target_cnt = cnt;
}

int target_cnt_get(void)
{
	return target_cnt;
}

int target_cnt_sub_and_fetch(void)
{
	return __sync_sub_and_fetch(&target_cnt, 1);
}


struct target *target_get(int i)
{
	return &target_array[i];
}





/*
 * for all targets
 */
int target_send_msg_to_all(struct msg_target_t *msg)
{
	int i, ret = 0;
	struct target *t;

	target_array_lock();
	for (i = 0; i < MAX_TARGET_COUNT; ++i) {
		if (target_use[i] == 0)
			continue;

		t = target_get(i);
		if (target_send_msg(t, msg))
			ret = 1;
	}
	target_array_unlock();

	return ret;
}

int target_send_terminate_to_all(void)
{
	int i, ret = 0;

	target_array_lock();
	for (i = 0; i < MAX_TARGET_COUNT; i++) {
		struct target *t;
		struct msg_target_t msg;

		if (target_use[i] == 0)
			continue;

		t = target_get(i);
		if (t->app_type == APP_TYPE_RUNNING) {
			msg.type = APP_MSG_STOP_WITHOUT_KILL;
			msg.length = 0;
		} else {
			msg.type = APP_MSG_STOP;
			msg.length = 0;
		}
		if (target_send_msg(t, &msg))
				ret = 1;
	}
	target_array_unlock();

	return ret;
}

void target_wait_all(void)
{
	int i;
	struct target *t;

	target_array_lock();
	for (i = 0; i < MAX_TARGET_COUNT; ++i) {
		LOGI("target_use [%d] = %d\n", i, target_use[i]);
		if (target_use[i] == 0)
			continue;

		t = target_get(i);
		if (close(t->socket) != 0) {
			LOGW("target socket already closed %u:%u\n", (unsigned int)t->pid, (unsigned int)t->ppid);
		}

		LOGI("join recv thread [%d] %u:%u is started\n", i, (unsigned int)t->pid, (unsigned int)t->ppid);
		target_wait(t);
		LOGI("join recv thread [%d] %u:%u done\n", i, (unsigned int)t->pid, (unsigned int)t->ppid);
	}

	target_array_unlock();

	for (i = 0; i < MAX_TARGET_COUNT; ++i) {
		if (target_use[i] == 0)
			continue;
		t = target_get(i);
		while (t->event_fd_released != 1) {
			LOGI("wait uninit [%d] %u:%u\n", i, t->pid, t->ppid);
			sleep(1);
		}
		LOGI("target destroy [%d] start\n", i);
		target_dtor(t);
		LOGI("target destroy [%d] done\n", i);
		target_use[i] = 0;
	}
}

void target_stop_all(void)
{
	int i;
	struct target *t;

	target_array_lock();
	for (i = 0; i < MAX_TARGET_COUNT; ++i) {
		if (target_use[i] == 0)
			continue;
		t = target_get(i);
		ecore_main_fd_handler_del(t->handler);
		t->event_fd_released = 1;
		target_cnt_sub_and_fetch();
	}
	target_array_unlock();
}

uint64_t target_get_total_alloc(pid_t pid)
{
	int i;
	uint64_t ret = 0;
	struct target *t;

	target_array_lock();
	for (i = 0; i < MAX_TARGET_COUNT; i++) {
		if (target_use[i] == 0)
			continue;

		t = target_get(i);
		if (target_get_pid(t) == pid) {
			ret += t->allocmem;
			/* TODO FIXME Split target libraries of different kind */
		}
	}
	target_array_unlock();

	return ret;
}
