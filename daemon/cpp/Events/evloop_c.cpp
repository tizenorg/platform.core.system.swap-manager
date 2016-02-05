#include <stdlib.h>

#include "EventLoop.h"
#include "Event.h"

#include "swap_debug.h"

static EventLoop evloop;
static std::mutex evloopMutex;

typedef std::lock_guard <std::mutex> LockGuardMutex;

extern "C" void evloop_init(void)
{
    LockGuardMutex lock(evloopMutex);

    if (evloop.isInit())
        return;

    evloop.init();
}

extern "C" void evloop_deinit(void)
{
    LockGuardMutex lock(evloopMutex);

    if (!evloop.isInit())
        return;

    evloop.deinit();
}

extern "C" void evloop_run(void)
{
    evloop.run();
}

extern "C" void evloop_stop(void)
{
    evloop.stop();
}

extern "C" void* evloop_add_event(int fd, void *data, evloop_event_cb cb)
{
    return (void *)evloop.addEvent(fd, data, cb);
}

extern "C" bool evloop_del_event(void *ev)
{
    return evloop.delEvent(ev);
}


