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

#include <sys/syscall.h>
#include <stdint.h>		// for uint64_t, int64_t
#include <pthread.h>	// for pthread_mutex_t
#include <stdarg.h>

#include "da_protocol.h"
#include "malloc_debug.h"
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
#define GETSTRERROR(err_code, buf)				\
	char buf[256];						\
	strerror_r(err_code, buf, sizeof(buf))

#define LOGE(...) do_log("ERR", __func__, __LINE__, __VA_ARGS__)
#define LOGW(...) do_log("WRN", __func__, __LINE__, __VA_ARGS__)

#ifdef USE_LOG_ONCE
	#define TOKENPASTE(x, y) x ## y
	#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
	#define LOG_ONCE_VAR TOKENPASTE2(log_once_var_, __LINE__)
	#define INIT_LOG_ONCE static char LOG_ONCE_VAR = 0

	#define LOG_ONCE(W_E,...)					\
		do {							\
			INIT_LOG_ONCE;					\
			if (LOG_ONCE_VAR == 0) {			\
				TOKENPASTE2(LOG, W_E)(__VA_ARGS__);	\
				LOG_ONCE_VAR = 1;			\
			}						\
		} while (0)

	#define LOG_ONCE_E(...) LOG_ONCE(E, __VA_ARGS__)
	#define LOG_ONCE_W(...) LOG_ONCE(W, __VA_ARGS__)
#else
	#define LOG_ONCE_W(...)
	#define LOG_ONCE_E(...)
#endif

static inline void do_log(const char *prefix, const char *funcname, int line, ...)
{
	va_list ap;
	const char *fmt;
	fprintf(stderr, "[%s][%f] PID %u:%u (%s:%d):", prefix, get_uptime(),
		(unsigned int)getpid(),
		(unsigned int)syscall(SYS_gettid),
		funcname, line);

	va_start(ap, line);
	fmt = va_arg(ap, const char *);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

	#ifdef NOLOGI
		#define LOGI(...)
		#define LOGI_(...)
	#else
		#define LOGI(...) do_log("INF", __func__, __LINE__, __VA_ARGS__)
		#define LOGI_(...)	do {		\
			fprintf(stderr, __VA_ARGS__);	\
			fflush(stderr);			\
		} while (0)

	#endif
#else
	#define GETSTRERROR(...)
	#define LOGI(...)
	#define LOGI_(...)
	#define LOGE(...)
	#define LOGW(...)
	#define LOG_ONCE_W(...)
	#define LOG_ONCE_E(...)

#endif



#ifdef __cplusplus
}
#endif

#endif // _DAEMON_DEBUG_H_
