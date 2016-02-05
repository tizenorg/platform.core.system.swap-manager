#ifndef __EVENT_LOOP_H__
#define __EVENT_LOOP_H__

#include <list>
#include <mutex>
#include "Event.h"
#include "EventLoop.h"

typedef std::lock_guard <std::mutex> lockGuardMutex;

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

    void stop();
    bool isRunning();

    void init();
    void deinit();
    bool isInit();

    Event* addEvent(int fd, void *data, evloop_event_cb cb);
    bool delEvent(int fd);
    bool delEvent(void *p);

    void run();

private:
    void enableRunning();
    bool checkEvents(struct epoll_event *events, int n);

    bool _running;
    int _efd;

    int _maxEvents;
    int _eventsCnt;
    std::list <Event *> _events;
    std::mutex _eventsMutex;
};

#endif // __EVENT_LOOP_H__
