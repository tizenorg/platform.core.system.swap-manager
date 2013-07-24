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

#include "da_protocol.h"

#include <linux/ioctl.h>

#ifndef __SWAP_IOCTL_COMMANDS_H__
#define __SWAP_IOCTL_COMMANDS_H__

#define SWAP_DRIVER_IOC_MAGIC 0xAF

struct buffer_initialize {
    size_t size;
    unsigned int count;
};
#define SWAP_DRIVER_BUFFER_INITIALIZE		_IOW(SWAP_DRIVER_IOC_MAGIC, 1, \
										     struct buffer_initialize *)
#define SWAP_DRIVER_BUFFER_UNINITIALIZE		_IO(SWAP_DRIVER_IOC_MAGIC, 2)
#define SWAP_DRIVER_NEXT_BUFFER_TO_READ		_IO(SWAP_DRIVER_IOC_MAGIC, 3)
#define SWAP_DRIVER_FLUSH_BUFFER			_IO(SWAP_DRIVER_IOC_MAGIC, 4)
#define SWAP_DRIVER_MSG						_IOW(SWAP_DRIVER_IOC_MAGIC, 5, \
										     void *)


int ioctl_send_msg(struct msg_t *msg);


#endif /* __SWAP_IOCTL_H__ */
