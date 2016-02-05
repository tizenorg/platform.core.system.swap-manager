#include <stdlib.h>

#include "EventLoop.h"
#include "Event.h"

#include "swap_debug.h"

static void *evloop = NULL;

extern "C" void evloop_init(void)
{
	if (evloop)
		return;

	EventLoop *loop = new EventLoop;

	loop->init();

	evloop =  (void *)loop;
}

extern "C" void evloop_deinit(void)
{
	if (!evloop)
		return;

	EventLoop *loop = (EventLoop *)evloop;
	loop->deinit();
	delete loop;

	return;
}

extern "C" void evloop_run(void)
{
	EventLoop *loop = (EventLoop *)evloop;
	if (!evloop) {
		LOGE("failed to start event loop\n");
		return;
	}

	return loop->run();
}

extern "C" void evloop_stop(void)
{
	EventLoop *loop = (EventLoop *)evloop;

	if (!loop) {
		return;
	}

	return loop->stop();
}

extern "C" void* evloop_add_event(int fd, void *data, int (*cb)(int, void *))
{
	if (!evloop) {
		LOGE("failed to add event to event loop\n");
		return NULL;
	}

	EventLoop *loop = (EventLoop *)evloop;
	if (!loop) {
		return NULL;
	}

	return (void *)loop->addEvent(fd, data, cb);
}

extern "C" bool evloop_del_event(void *ev)
{
	if (!evloop) {
		LOGE("failed to del event from event loop\n");
		return false;
	}

	EventLoop *loop = (EventLoop *)evloop;

	return loop->delEvent(ev);
}

extern "C" bool evloop_del_event_by_fd(int fd)
{
	if (!evloop) {
		LOGE("failed to del event from event loop\n");
		return false;
	}

	EventLoop *loop = (EventLoop *)evloop;

	return loop->delEvent(fd);
}


