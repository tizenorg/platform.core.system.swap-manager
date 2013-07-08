
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
#define DEB_PRINTBUF

#ifndef DEB_PRINTBUF
	#define printBuf() do {} while(0)
#endif

//threads debug
//#define THREAD_SAMPLING_DEBUG
//#define THREAD_REPLAY_DEBUG

#ifdef THREAD_SAMPLING_DEBUG
	#define LOGI_th_samp LOGI
#else
	#define LOGI_th_samp(...)	do{} while(0)
#endif


#ifdef THREAD_REPLAY_DEBUG
	#define LOGI_th_rep LOGI
#else
	#define LOGI_th_rep(...)	do{} while(0)
#endif


/*
 * END DEBUG DEFINES
 */
//#define DEBUG

#ifdef DEBUG
//	#define LOGI(...)	do{ fprintf(stderr, "[INF] (%s):", __FUNCTION__); fflush(stderr); fprintf(stderr, __VA_ARGS__ ); fflush(stderr); usleep(1000000);} while(0)
	#define LOGI(...)	do{ fprintf(stderr, "[INF] (%s):", __FUNCTION__); fflush(stderr); fprintf(stderr, __VA_ARGS__ ); fflush(stderr); } while(0)
//	#define LOGI(...)	do{} while(0)
	#define LOGI_(...)	do{ fprintf(stderr, __VA_ARGS__ ); fflush(stderr); } while(0)
	//#define LOGI_(...)	do{} while(0)

//	#define LOGE(...)	do{ fprintf(stderr, "[ERR] (%s):", __FUNCTION__); fflush(stderr); fprintf(stderr, __VA_ARGS__ ); fflush(stderr); usleep(1000000);} while(0)
	#define LOGE(...)	do{ fprintf(stderr, "[ERR] (%s):", __FUNCTION__); fflush(stderr); fprintf(stderr, __VA_ARGS__ ); fflush(stderr); } while(0)
	#define LOGW(...)	do{ fprintf(stderr, "[WRN] (%s):", __FUNCTION__); fflush(stderr); fprintf(stderr, __VA_ARGS__ ); fflush(stderr); } while(0)


#else
	#define LOGI(...)	do{} while(0)
	#define LOGI_(...)	do{} while(0)
	#define LOGE(...)	do{} while(0)
	#define LOGW(...)	do{} while(0)
#endif

// DEBUGS
void printBuf (char * buf, int len);

#ifdef __cplusplus
}
#endif

#endif // _DAEMON_DEBUG_H_
