#ifndef _TARGER_H_
#define _TARGER_H_


#include <inttypes.h>
#include <pthread.h>

#include "da_protocol.h"


struct target {
	enum app_type_t app_type;	/* calculated when connecting */
	int64_t	allocmem;		/* written only by recv thread */
	pid_t pid;			/* written only by recv thread */
	pid_t ppid;			/* written only by recv thread */
	int socket;			/* written only by main thread */
	pthread_t recv_thread;		/* written only by main thread */
	int event_fd;			/* for thread communication
					 * (from recv thread to main thread) */
	int initial_log;		/* written only by main thread */
};


#endif /* _TARGER_H_ */
