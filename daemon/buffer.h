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


#ifndef _BUFFER_
#define _BUFFER_

#include "da_protocol.h"

#define USER_EVENT_FILENAME "/sys/kernel/debug/swap/writer/raw"
#define BUF_FILENAME "/dev/swap_device"
#define INST_PID_FILENAME "/sys/kernel/debug/swap/us_manager/tasks"

int init_buf(void);
void exit_buf(void);
void flush_buf(void);
void wake_up_buf(void);
int write_to_buf(struct msg_data_t *msg);

#endif /* _BUFFER_ */
