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


#ifndef _THREAD_H_
#define _THREAD_H_


struct thread;


struct thread *thread_ctor(void);
void thread_dtor(struct thread *t);


int thread_start(struct thread *t, void *(*func) (void *), void *data);
int target_wait(struct target *t);


#endif /* _THREAD_H_ */
