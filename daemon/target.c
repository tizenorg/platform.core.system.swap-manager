#include "target.h"

#include "daemon.h"	// for manager (it is need delete)


int target_start(struct target *t, void *(*start_routine) (void *))
{
	return pthread_create(&t->recv_thread, NULL, start_routine, (void *)t);
}

int target_wait(struct target *t)
{
	return pthread_join(t->recv_thread, NULL);
}





static int target_cnt = 0;
static struct target target_array[MAX_TARGET_COUNT] = {0};


void target_cnt_set(int cnt)
{
	target_cnt = cnt;
}

int target_cnt_get(void)
{
	return target_cnt;
}

int target_cnt_sub_and_fetch(void)
{
	return __sync_sub_and_fetch(&target_cnt, 1);
}


struct target *target_get(int i)
{
	return &target_array[i];
}
