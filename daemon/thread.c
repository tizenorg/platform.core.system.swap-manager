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


#include <pthread.h>
#include <errno.h>


#include "swap_debug.h"


struct thread {
	pthread_t thread;
	pthread_mutex_t mutex;
	unsigned run_flag:1;
};


struct thread *thread_ctor(void)
{
	struct thread *t;

	t = malloc(sizeof(*t));
	if (t) {
		t->run_flag = 0;
		pthread_mutex_init(&t->mutex, NULL);
	}

	return t;
}

void thread_dtor(struct thread *t)
{
	if (t->run_flag == 1)
		SWAP_LOGE("hanging thread: t=%p\n", t);

	free(t);
}

int thread_start(struct thread *t, void *(*func) (void *), void *data)
{
	int ret = EBUSY;

	pthread_mutex_lock(&t->mutex);
	if (t->run_flag == 0) {
		ret = pthread_create(&t->thread, NULL, func, data);
		if (ret == 0)
			t->run_flag = 1;
	}
	pthread_mutex_unlock(&t->mutex);

	return ret;
}

int thread_wait(struct thread *t)
{
	int ret = ESRCH;

	pthread_mutex_lock(&t->mutex);
	if (t->run_flag == 1) {
		ret = pthread_join(t->thread, NULL);
		t->run_flag = 0;
	}
	pthread_mutex_unlock(&t->mutex);

	return ret;
}
