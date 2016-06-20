#ifndef DAEMON__BENCHMARK__BENCHMARK_H
#define DAEMON__BENCHMARK__BENCHMARK_H

#include <inttypes.h>

struct SystemBenchmark
{
	uint64_t avg_syscall_overhead;
};

extern "C" void performBenchmark(SystemBenchmark * benchmark);


#endif // DAEMON__BENCHMARK__BENCHMARK_H
