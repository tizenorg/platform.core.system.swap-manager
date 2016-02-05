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

using std::cout;
using std::endl;
using std::list;
using std::vector;

typedef std::lock_guard <std::mutex> lock_guard_mutex;

EventLoop::EventLoop()
{
	running = false;
	efd_ = -1;
	max_events_ = 16;
	events_cnt_ = 0;
}

EventLoop::~EventLoop()
{
	running = false;
	efd_ = -1;
	max_events_ = 0;
	events_cnt_ = 0;
}

bool EventLoop::isRunning()
{
	return running;
}

void EventLoop::enableRunning_()
{
	running = true;
}

void EventLoop::stop()
{
	running = false;
}

void EventLoop::init()
{
	enableRunning_();

	efd_ = epoll_create1(0);
	if (efd_ < 0) {
		LOGE("failed to create epoll descriptor");
	}
}

void EventLoop::deinit()
{
	int n;
	list<Event *>::iterator it;

	stop();

	lock_guard_mutex lock(events_mutex_);

	for (it = events_.begin(); it != events_.end(); ++it) {
		Event *ev = *it;
		LOGI("remove event %p %d\n", ev, ev->fdShow());
		n = epoll_ctl(efd_, EPOLL_CTL_DEL, ev->fdShow(), NULL);
		if (n == 0) {
			it = events_.erase(it);
			delete ev;
			events_cnt_--;
		}
	}

	close(efd_);
}

Event* EventLoop::addEvent(int fd, void *data, int (*cb)(int, void*))
{
	int n;
	struct epoll_event event;

	lock_guard_mutex lock(events_mutex_);

	Event *ev = new Event(fd, data, cb);
	if (!ev)
		return NULL;

	event.data.ptr = (void *)ev;
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

	n = epoll_ctl(efd_, EPOLL_CTL_ADD, fd, &event);
	if (n < 0) {
		delete ev;
		return NULL;
	}

	events_.push_back(ev);
	events_cnt_++;

	if (events_cnt_ >= max_events_)
		max_events_ += 16;

	return ev;
}

bool EventLoop::delEvent(int fd)
{
	int n;
	list<Event *>::iterator it;

	lock_guard_mutex lock(events_mutex_);

	for (it = events_.begin(); it != events_.end(); ++it) {
		Event *ev = *it;
                LOGI("ev %p %d\n", ev, ev->fdShow());
		if (fd == ev->fdShow()) {
			LOGI("remove event %p %d\n", ev, ev->fdShow());
			n = epoll_ctl(efd_, EPOLL_CTL_DEL, fd, NULL);
			if (n == 0) {
				it = events_.erase(it);
				delete ev;
				events_cnt_--;
			}
		}
	}
	return true;
}

bool EventLoop::delEvent(void *p)
{
	int n;
	list<Event *>::iterator it;

	lock_guard_mutex lock(events_mutex_);

	for (it = events_.begin(); it != events_.end(); ++it) {
		Event *ev = *it;
		if (ev == p) {
			LOGI("remove event %p %d\n", ev, ev->fdShow());
			n = epoll_ctl(efd_, EPOLL_CTL_DEL, ev->fdShow(), NULL);
			if (n == 0) {
				it = events_.erase(it);
				delete ev;
				events_cnt_--;
			}
		}
	}
	return true;
}

bool EventLoop::checkEvents_(struct epoll_event *events, int n)
{
	for (int i = 0; i < n; i++) {
		Event *ev;

		ev = (Event *)events[i].data.ptr;
		if (!ev)
			return false;

		if (events[i].events & EPOLLRDHUP) {

		} else if (events[i].events & EPOLLERR) {

		} else if(events[i].events & EPOLLHUP) {

		} else if ((events[i].events & EPOLLIN) ||
			   (events[i].events & EPOLLOUT)) {
			ev->doCallback();
		}
	}

	return true;

}

void EventLoop::run()
{
	std::vector<struct epoll_event> arr(max_events_);

	while (isRunning()) {
		int n = epoll_wait(efd_, arr.data(), max_events_, 60000);
		if (n > 0)
			checkEvents_(arr.data(), n);
	}

	return;
}
