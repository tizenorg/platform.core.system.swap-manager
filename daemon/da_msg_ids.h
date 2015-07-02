/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
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

#define MSG_ID_LIST \
X(NMSG_UNKNOWN,			0x0000) \
X(NMSG_VERSION,			0x0001) \
X(NMSG_START,			0x0002) \
X(NMSG_STOP,			0x0003) \
X(NMSG_CONFIG,			0x0004) \
X(NMSG_BINARY_INFO,		0x0005) \
X(NMSG_GET_TARGET_INFO,		0x0007) \
X(NMSG_SWAP_INST_ADD,		0x0008) \
X(NMSG_SWAP_INST_REMOVE,	0x0009) \
X(NMSG_GET_PROBE_MAP,		0x000A) \
X(NMSG_KEEP_ALIVE,		0x000F) \
X(NMSG_GET_SCREENSHOT,		0x0010) \
X(NMSG_GET_PROCESS_ADD_INFO,	0x0011) \
X(NMSG_GET_UI_HIERARCHY,	0x0012) \
X(NMSG_GET_UI_SCREENSHOT,	0x0013) \
X(NMSG_GET_REAL_PATH,		0x0020)

#define DATA_MSG_ID_LIST \
X(NMSG_PROCESS_INFO,		0x0001) /* target process info */ \
X(NMSG_TERMINATE,		0x0002) /* terminate */ \
X(NMSG_ERROR,			0x0003) /* error message */ \
X(NMSG_SAMPLE,			0x0004) /* N	10ms */ \
X(NMSG_SYSTEM,			0x0005) /* N	10~1000ms	DaData, start sending immediately after start message from host, first system message time is tracing start time */ \
X(NMSG_IMAGE,			0x0006) /* N	irregular	image */ \
X(NMSG_RECORD,			0x0007) /* N	irregular	replay event */ \
X(NMSG_FUNCTION_ENTRY,		0x0008) /* N	irregular	swap instrumentation, Instrumented functions by AppInst and LibInst */ \
X(NMSG_FUNCTION_EXIT,		0x0009) /* N	irregular	swap instrumentation, Instrumented functions by AppInst and LibInst */ \
X(NMSG_CONTEXT_SWITCH_ENTRY,	0x0010) /* N	irregular	swap instrumentation for kernel */ \
X(NMSG_CONTEXT_SWITCH_EXIT,	0x0011) /* N	irregular	swap instrumentation for kernel */ \
X(NMSG_UI_HIERARCHY,		0x0021) /* N	irregular	ui hierarchy */

