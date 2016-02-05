#include <stdlib.h>

#include "EventLoop.h"
#include "Event.h"

Event::Event(int fd, void* data, evloop_event_cb cb)
{
	fd_ = fd;
	data_ = data;
	cb_ = cb;
	loop_ = NULL;
}

Event::~Event()
{
	fd_ = -1;
	data_ = NULL;
	cb_ = NULL;
	loop_ = NULL;
}

int Event::fdShow()
{
	return fd_;
}
/*
void* Event::getLoop_()
{
	return loop_;
}
*/
int Event::doCallback()
{
	return cb_(fd_, data_);
}

