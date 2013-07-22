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


#ifndef _DA_DATA_H_
#define _DA_DATA_H_
int fill_data_msg_head (struct msg_data_t *data, uint32_t msgid,
						uint32_t seq, uint32_t len);
//allocate memory, need free!!!
struct msg_data_t *gen_message_terminate(uint32_t id);

//allocatr memory, need free!!!
struct msg_data_t *gen_message_event(
		      struct input_event *events,
		      uint32_t events_count,
		      uint32_t id);


struct msg_data_t *gen_message_error(const char * err_msg);

struct msg_data_t *gen_message_event(
		      struct input_event *events,
		      uint32_t events_count,
		      uint32_t id);

int reset_data_msg(struct msg_data_t *data);

//DEBUGS
int print_sys_info(struct system_info_t * sys_info);
#endif //#ifndef _DA_DATA_H_
