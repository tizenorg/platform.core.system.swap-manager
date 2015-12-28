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

#ifndef _UI_VIEWER_LIB_
#define _UI_VIEWER_LIB_

#include <stdlib.h>
#include <pthread.h>

typedef struct
{
	int daemonSock;
	pthread_mutex_t sockMutex;
} __socketInfo;

typedef struct
{
	__socketInfo		socket;
	uint64_t		optionflag;
} __traceInfo;


#endif /* _UI_VIEWER_LIB_ */