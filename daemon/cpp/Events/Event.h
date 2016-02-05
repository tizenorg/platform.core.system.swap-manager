#ifndef __EVENT_H__
#define __EVENT_H__

#include "evloop.h"

class Event
{
public:
    Event(int fd, void* data, evloop_event_cb cb) :
          _fd(fd),
          _data(data),
          _cb(cb)
    {};

    bool doCallback()
    {
        return _cb(_fd, _data);
    };

    int fd()
    {
        return _fd;
    };

private:
    int _fd;
    void *_data;
    evloop_event_cb _cb;
};

#endif // __EVENT_H__
