/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
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
#ifndef __DA_PROTOCOL_INST__
#define __DA_PROTOCOL_INST__

int parse_us_inst_func(struct msg_buf_t *msg, struct probe_list_t **dest);

int parse_func_inst_list(struct msg_buf_t *msg,
				struct data_list_t *dest);

int parse_inst_lib(struct msg_buf_t *msg, struct lib_list_t **dest);

int parse_lib_inst_list(struct msg_buf_t *msg,
			uint32_t *num,
			struct lib_list_t **lib_list);

int parse_inst_app(struct msg_buf_t *msg, struct app_list_t **dest);

int parse_app_inst_list(struct msg_buf_t *msg,
			uint32_t *num,
			struct app_list_t **app_list);

#endif /* __DA_PROTOCOL_INST__ */
