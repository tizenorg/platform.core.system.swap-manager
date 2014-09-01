#ifndef _TARGER_H_
#define _TARGER_H_


#include <inttypes.h>
#include <pthread.h>

#include <Ecore.h>

#include "da_protocol.h"


#define UNKNOWN_PID		((pid_t)(-1))
#define UNKNOWN_FD		(-1)


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
	Ecore_Fd_Handler *handler;	/* calculated when connecting */
};


struct target *target_ctor(void);
void target_dtor(struct target *t);

int target_accept(struct target *t, int sockfd);

int target_send_msg(struct target *t, struct msg_target_t *msg);
int target_recv_msg(struct target *t, struct msg_target_t *msg);


int target_start(struct target *t, void *(*start_routine) (void *));
int target_wait(struct target *t);


pid_t target_get_pid(struct target *t);
void target_set_pid(struct target *t, pid_t pid);

pid_t target_get_ppid(struct target *t);
void target_set_ppid(struct target *t, pid_t ppid);


void target_cnt_set(int cnt);
int target_cnt_get(void);
int target_cnt_sub_and_fetch(void);

struct target *target_get(int i);

/* for all targets */
int target_send_msg_to_all(struct msg_target_t *msg);
void target_wait_all(void);
uint64_t target_get_total_alloc(pid_t pid);


#endif /* _TARGER_H_ */
