#include <stdlib.h>

#include "EventLoop.h"
#include "Event.h"

#include "swap_debug.h"

static EventLoop evloop;
static std::mutex evloop_mutex;

extern "C" void evloop_init(void)
{
    lock_guard_mutex lock(evloop_mutex);

    if (evloop.isInit())
        return;

    evloop.init();
}

extern "C" void evloop_deinit(void)
{
    lock_guard_mutex lock(evloop_mutex);

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


