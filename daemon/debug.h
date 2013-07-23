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

#include "da_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DEBUG DEFINES
 */
//#define DEB_PRINTBUF
//#define PARSE_DEBUG_ON
//threads debug
//#define THREAD_SAMPLING_DEBUG
//#define THREAD_REPLAY_DEBUG


//DEBUG ON PARSING
#ifdef PARSE_DEBUG_ON
#define parse_deb LOGI
#else
#define parse_deb(...) do{}while(0)
#endif /*parse_on*/

//PRINT BUFFER DEBUG
void printBuf (char * buf, int len);

//THREAD SAMPLING DEBUG
#ifdef THREAD_SAMPLING_DEBUG
	#define LOGI_th_samp LOGI
#else
	#define LOGI_th_samp(...)	do{} while(0)
#endif

//THREAD REPLAY DEBUG
#ifdef THREAD_REPLAY_DEBUG
	#define LOGI_th_rep LOGI
#else
	#define LOGI_th_rep(...)	do{} while(0)
#endif


/*
 * END DEBUG DEFINES
 */
#ifdef DEBUG

#ifdef NOLOGI
	#define LOGI(...)	do{} while(0)
	#define LOGI_(...)	do{} while(0)
#else
//	#define LOGI(...)	do{ fprintf(stderr, "[INF] (%s):", __FUNCTION__); fflush(stderr); fprintf(stderr, __VA_ARGS__ ); fflush(stderr); usleep(100000);} while(0)
	#define LOGI(...)	do{ fprintf(stderr, "[INF] (%s):", __FUNCTION__); fflush(stderr); fprintf(stderr, __VA_ARGS__ ); fflush(stderr); } while(0)
//	#define LOGI(...)	do{} while(0)
	#define LOGI_(...)	do{ fprintf(stderr, __VA_ARGS__ ); fflush(stderr); } while(0)
	//#define LOGI_(...)	do{} while(0)
#endif

//	#define LOGE(...)	do{ fprintf(stderr, "[ERR] (%s):", __FUNCTION__); fflush(stderr); fprintf(stderr, __VA_ARGS__ ); fflush(stderr); usleep(1000000);} while(0)
	#define LOGE(...)	do{ fprintf(stderr, "[ERR] (%s):", __FUNCTION__); fflush(stderr); fprintf(stderr, __VA_ARGS__ ); fflush(stderr); } while(0)
	#define LOGW(...)	do{ fprintf(stderr, "[WRN] (%s):", __FUNCTION__); fflush(stderr); fprintf(stderr, __VA_ARGS__ ); fflush(stderr); } while(0)


#else
	#define LOGI(...)	do{} while(0)
	#define LOGI_(...)	do{} while(0)
	#define LOGE(...)	do{} while(0)
	#define LOGW(...)	do{} while(0)
#endif



#ifdef __cplusplus
}
#endif

#endif // _DAEMON_DEBUG_H_
