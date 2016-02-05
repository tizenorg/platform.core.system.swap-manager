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
    _maxEvents = 16;
    _eventsCnt = 0;
}

EventLoop::~EventLoop()
{
    if (isInit())
        LOGE("remove initialized EventLoop object");
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
    std::lock_guard <std::mutex> lock(_eventsMutex);
    for (std::list<Event *>::iterator it = _events.begin();
         it != _events.end();
         ++it)
    {
        Event *ev = *it;
        LOGI("remove event %p %d\n", ev, ev->fd());
        int n = epoll_ctl(_efd, EPOLL_CTL_DEL, ev->fd(), NULL);
        if (n == 0) {
            LOGI("event %p %d removed\n", ev, ev->fd());
            it = _events.erase(it);
            delete ev;
            _eventsCnt--;
        }
    }

    close(_efd);
    _efd = -1;
}

Event* EventLoop::addEvent(int fd, void *data, evloop_event_cb cb)
{
    Event *ev = new Event(fd, data, cb);
    if (!ev)
        return NULL;

    struct epoll_event event;
    event.data.ptr = (void *)ev;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

    int n = epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &event);
    if (n < 0) {
        delete ev;
        return NULL;
    }

    LOGI("add event %p %d\n", ev, ev->fd());
    std::lock_guard <std::mutex> lock(_eventsMutex);
    _events.push_back(ev);
    _eventsCnt++;

    if (_eventsCnt >= _maxEvents)
        _maxEvents += 16;

    return ev;
}

bool EventLoop::delEvent(void *p)
{
    std::lock_guard <std::mutex> lock(_eventsMutex);
    for (std::list<Event *>::iterator it = _events.begin();
         it != _events.end();
         ++it)
    {
        Event *ev = *it;
        if (ev == p) {
            LOGI("remove event %p %d\n", ev, ev->fd());
            int n = epoll_ctl(_efd, EPOLL_CTL_DEL, ev->fd(), NULL);
            if (n == 0) {
                it = _events.erase(it);
                delete ev;
                _eventsCnt--;
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
            LOGI("event %p %d remote hangup\n", ev, ev->fd());
        } else if (events[i].events & EPOLLERR) {
            LOGE("event %p %d epoll error\n", ev, ev->fd());
        } else if(events[i].events & EPOLLHUP) {
            LOGI("event %p %d hangup\n", ev, ev->fd());
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
        std::vector<struct epoll_event> arr(_maxEvents);

        int n = epoll_wait(_efd, arr.data(), _maxEvents, 60000);
        if (n > 0)
            checkEvents(arr.data(), n);
    }
}
