#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "Performeter.h"
#include "Benchmark.h"


#define SYSCALL_MEASURE_COUNT 1000

extern "C" void performBenchmark(SystemBenchmark * benchmark)
{
	if (NULL == benchmark)
		return;

	memset(benchmark, 0, sizeof(*benchmark));
	Performeter syscall_overhead;

	{
		PerfCounter counter(syscall_overhead);
		for(size_t k = 0; k != SYSCALL_MEASURE_COUNT; ++k)
			getpid();
	}

	benchmark->avg_syscall_overhead = syscall_overhead.get();

}