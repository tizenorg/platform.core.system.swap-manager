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


/* SWAP Device ioctl commands */

#include "ioctl_commands.h"
#include "swap_debug.h"
#include "da_protocol.h"
#include "daemon.h"

#include <errno.h>

//send message to device
int ioctl_send_msg(struct msg_t *msg)
{
	SWAP_LOGI("write to device\n");
	if (ioctl(manager.buf_fd, SWAP_DRIVER_MSG, msg) == -1) {
		GETSTRERROR(errno, buf);
		SWAP_LOGE("write to device: %s\n", buf);
		return 1;
	}
	return 0;
}
