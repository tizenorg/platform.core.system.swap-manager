#ifndef __EVENT_H__
#define __EVENT_H__

#include "EventLoop.h"

typedef bool (*evloop_event_cb)(int, void *);

class Event
{
public:
    Event(int fd, void* data, evloop_event_cb cb);
    ~Event();
    int doCallback();
    int fdShow();

private:
    void* getLoop();
    int _fd;
    void *_data;
    evloop_event_cb _cb;
    void *_loop;
};

#endif // __EVENT_H__
