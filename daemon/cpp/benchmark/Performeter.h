#ifndef DAEMON__BENCHMARK__PERFORMETER_H
#define DAEMON__BENCHMARK__PERFORMETER_H

#include <time.h>
#include <inttypes.h>

typedef uint64_t clock_ns_type;

struct Performeter
{
	Performeter()
		: _duration(0)
		, _count(0)
	{}

	~Performeter()
	{
	}

	void add(clock_ns_type interval) { _duration += interval; ++_count;}
	
    clock_ns_type get() const { return _duration; }

protected:
	clock_ns_type _duration;
	uint32_t _count;
};


struct PerfCounter
{
	PerfCounter(Performeter& perf)
		: _perf(perf)
		, _start(get_timestamp())
	{
	}
	~PerfCounter()
	{
		_perf.add(get_timestamp() - _start);
	}
protected:
	static inline clock_ns_type get_timestamp()
	{
        struct timespec ts = {0, 0};
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return (clock_ns_type)ts.tv_sec * 1000000000 + ts.tv_nsec;
	}
	Performeter& _perf;
	clock_ns_type _start;
};


#endif //DAEMON__BENCHMARK__PERFORMETER_H
