#ifndef __EVENT_LOOP_H__
#define __EVENT_LOOP_H__

#include <list>
#include <mutex>
#include "Event.h"
#include "EventLoop.h"

using std::list;
using std::mutex;

class EventLoop
{
public:
	EventLoop();
	~EventLoop();

	void stop();
	bool isRunning();

	void init();
	void deinit();

	Event* addEvent(int fd, void *data, int (*cb)(int, void*));
	bool delEvent(int fd);
	bool delEvent(void *p);

	void run();
	void dump();

private:
	void enableRunning_();
	bool checkEvents_(struct epoll_event *events, int n);
	bool running;
	int efd_;
	int max_events_;
	int events_cnt_;
	list <Event *> events_;
	mutex events_mutex_;
};

#endif // __EVENT_LOOP_H__
