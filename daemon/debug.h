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


#ifndef _DAEMON_DEBUG_H_
#define _DAEMON_DEBUG_H_

#include <stdint.h>		// for uint64_t, int64_t
#include <pthread.h>	// for pthread_mutex_t
#include <stdarg.h>

#include "da_protocol.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define printBuf(buf, len) print_buf(buf, len, __func__)
void print_buf(char *buf, int len, const char *info);

#ifdef PARSE_DEBUG_ON
	#define parse_deb LOGI
#else
	#define parse_deb(...)
#endif

#ifdef THREAD_SAMPLING_DEBUG
	#define LOGI_th_samp LOGI
#else
	#define LOGI_th_samp(...)
#endif

#ifdef THREAD_REPLAY_DEBUG
	#define LOGI_th_rep LOGI
#else
	#define LOGI_th_rep(...)
#endif

#ifdef DEBUG
#define LOGE(...) do_log("ERR", __func__, __VA_ARGS__)
#define LOGW(...) do_log("WRN", __func__, __VA_ARGS__)

static inline void do_log(const char *prefix, const char *funcname, ...)
{
	va_list ap;
	const char *fmt;
	fprintf(stderr, "[%s][%f] (%s):", prefix, get_uptime(), funcname);

	va_start(ap, funcname);
	fmt = va_arg(ap, const char *);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

	#ifdef NOLOGI
		#define LOGI(...)
		#define LOGI_(...)
	#else
		#define LOGI(...) do_log("INF", __func__, __VA_ARGS__)
		#define LOGI_(...)	do {		\
			fprintf(stderr, __VA_ARGS__);	\
			fflush(stderr);			\
		} while (0)

	#endif
#else
	#define LOGI(...)
	#define LOGI_(...)
	#define LOGE(...)
	#define LOGW(...)
#endif



#ifdef __cplusplus
}
#endif

#endif // _DAEMON_DEBUG_H_
