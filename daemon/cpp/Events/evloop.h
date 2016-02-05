#ifndef __EVLOOP_H__
#define __EVLOOP_H__

#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*evloop_event_cb)(int, void *);

void evloop_init(void);
void evloop_deinit(void);
void evloop_run(void);
void evloop_stop(void);
void* evloop_add_event(int fd, void *data, evloop_event_cb cb);
bool evloop_del_event(void *p);

#ifdef __cplusplus
}
#endif
#endif //__EVLOOP_H__




