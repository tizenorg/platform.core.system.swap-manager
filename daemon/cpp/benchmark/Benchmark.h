#ifndef DAEMON__BENCHMARK__BENCHMARK_H
#define DAEMON__BENCHMARK__BENCHMARK_H

#include <inttypes.h>
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else 
#define EXTERN_C
#endif

struct SystemBenchmark
{
	uint64_t avg_syscall_overhead;
};

EXTERN_C void performBenchmark(struct SystemBenchmark * benchmark);


#endif // DAEMON__BENCHMARK__BENCHMARK_H
