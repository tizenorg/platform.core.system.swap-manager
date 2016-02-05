#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>

#include <iostream>
#include <list>
#include <vector>
#include <mutex>

#include "swap_debug.h"
#include "EventLoop.h"
#include "Event.h"

EventLoop::EventLoop()
{
    _running = false;
    _efd = -1;
    _max_events = 16;
    _events_cnt = 0;
}

EventLoop::~EventLoop()
{
    _running = false;
    _efd = -1;
    _max_events = 0;
    _events_cnt = 0;
}

bool EventLoop::isRunning()
{
    return _running;
}

void EventLoop::enableRunning()
{
    _running = true;
}

void EventLoop::stop()
{
    _running = false;
}

bool EventLoop::isInit()
{
    return _efd >= 0;
}

void EventLoop::init()
{
    _efd = epoll_create1(0);
    if (_efd < 0) {
        LOGE("failed to create epoll descriptor");
        return;
    }

    enableRunning();
}

void EventLoop::deinit()
{
    stop();
    lock_guard_mutex lock(_events_mutex);
    for (std::list<Event *>::iterator it = _events.begin();
         it != _events.end();
         ++it)
    {
        Event *ev = *it;
        LOGI("remove event %p %d\n", ev, ev->fdShow());
        int n = epoll_ctl(_efd, EPOLL_CTL_DEL, ev->fdShow(), NULL);
        if (n == 0) {
            LOGI("event %p %d removed\n", ev, ev->fdShow());
            it = _events.erase(it);
            delete ev;
            _events_cnt--;
        }
    }

    close(_efd);
    _efd = -1;
}

Event* EventLoop::addEvent(int fd, void *data, evloop_event_cb cb)
{
    struct epoll_event event;

    lock_guard_mutex lock(_events_mutex);

    Event *ev = new Event(fd, data, cb);
    if (!ev)
        return NULL;

    event.data.ptr = (void *)ev;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

    int n = epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &event);
    if (n < 0) {
        delete ev;
        return NULL;
    }

    LOGI("add event %p %d\n", ev, ev->fdShow());
    _events.push_back(ev);
    _events_cnt++;

    if (_events_cnt >= _max_events)
        _max_events += 16;

    return ev;
}

bool EventLoop::delEvent(void *p)
{
    int n;
    std::list<Event *>::iterator it;

    lock_guard_mutex lock(_events_mutex);

    for (std::list<Event *>::iterator it = _events.begin();
         it != _events.end();
         ++it)
    {
        Event *ev = *it;
        if (ev == p) {
            LOGI("remove event %p %d\n", ev, ev->fdShow());
            n = epoll_ctl(_efd, EPOLL_CTL_DEL, ev->fdShow(), NULL);
            if (n == 0) {
                it = _events.erase(it);
                delete ev;
                _events_cnt--;
            }
        }
    }
    return true;
}

bool EventLoop::checkEvents(struct epoll_event *events, int n)
{
    for (int i = 0; i < n; i++) {
        Event *ev = (Event *)events[i].data.ptr;
        if (!ev)
            return false;

        if (events[i].events & EPOLLRDHUP) {
            LOGI("event %p %d remote hangup\n", ev, ev->fdShow());
        } else if (events[i].events & EPOLLERR) {
            LOGE("event %p %d epoll error\n", ev, ev->fdShow());
        } else if(events[i].events & EPOLLHUP) {
            LOGI("event %p %d hangup\n", ev, ev->fdShow());
        } else if ((events[i].events & EPOLLIN) ||
               (events[i].events & EPOLLOUT)) {
            ev->doCallback();
        }
    }

    return true;
}

void EventLoop::run()
{
    while (isRunning()) {
        std::vector<struct epoll_event> arr(_max_events);

        int n = epoll_wait(_efd, arr.data(), _max_events, 60000);
        if (n > 0)
            checkEvents(arr.data(), n);
    }
}
