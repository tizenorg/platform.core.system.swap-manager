#include <stdlib.h>

#include "EventLoop.h"
#include "Event.h"

Event::Event(int fd, void* data, evloop_event_cb cb)
{
    _fd = fd;
    _data = data;
    _cb = cb;
    _loop = NULL;
}

Event::~Event()
{
    _fd = -1;
    _data = NULL;
    _cb = NULL;
    _loop = NULL;
}

int Event::fdShow()
{
    return _fd;
}

int Event::doCallback()
{
    return _cb(_fd, _data);
}

