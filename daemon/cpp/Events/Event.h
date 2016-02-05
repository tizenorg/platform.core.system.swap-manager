#ifndef __EVENT_H__
#define __EVENT_H__

#include "EventLoop.h"

typedef int(*evloop_event_cb)(int, void *);

class Event
{
public:
	Event(int fd, void* data, int (*cb)(int, void*));
	~Event();
	int doCallback();
	int fdShow();

private:
	void* getLoop_();

	int fd_;
	void *data_;
	evloop_event_cb cb_;
	void *loop_;
//	bool ev_changed;
//	size_t max_events;
//	struct epoll_event *events;
//	struct nlist_head ev_head;
};

#endif // __EVENT_H__
