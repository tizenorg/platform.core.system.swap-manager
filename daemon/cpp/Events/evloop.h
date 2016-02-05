#ifndef __EVLOOP_H__
#define __EVLOOP_H__

#include "stdbool.h"

void evloop_init(void);
void evloop_deinit(void);
void evloop_run(void);
void evloop_stop(void);
bool evloop_del_event_by_fd(int fd);
bool evloop_del_event(void *p);

#endif //__EVLOOP_H__




