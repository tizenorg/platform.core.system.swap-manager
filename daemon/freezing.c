#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "freezing.h"
#include "debug.h"

#define FREEZER_SUBGROUP "/sys/fs/cgroup/freezer/swap"
#define FREEZER_TASKS FREEZER_SUBGROUP "/tasks"
#define FREEZER_STATE FREEZER_SUBGROUP "/freezer.state"

#define US_MANAGER_TASKS "/sys/kernel/debug/swap/us_manager/tasks"

static FILE *f_tasks_fd = NULL, *f_state_fd = NULL, *f_us_man_fd = NULL;
static const char *freezer_frozen = "FROZEN";
static const char *freezer_thawed = "THAWED";
static char freezer_enabled = 0;

static int open_freezer_files(void)
{
	f_tasks_fd = fopen(FREEZER_TASKS, "w+");
	if (f_tasks_fd == NULL)
		return -1;

	f_state_fd = fopen(FREEZER_STATE, "w");
	if (f_state_fd == NULL)
		return -1;

	f_us_man_fd = fopen(US_MANAGER_TASKS, "r");
	if (f_us_man_fd == NULL)
		return -1;

	return 0;
}

static int close_freezer_files(void)
{
	int res = 0;

	if (f_tasks_fd != NULL)
		res = fclose(f_tasks_fd);

	if (f_state_fd != NULL)
		res |= fclose(f_state_fd);

	if (f_us_man_fd != NULL)
		res |= fclose(f_us_man_fd);

	return res;
}

int create_freezer_subgroup(void)
{
	struct stat st;
	int res = 0;

	if (stat(FREEZER_SUBGROUP, &st) == -1) {
		res = mkdir(FREEZER_SUBGROUP, 0700);
		if (res)
			return res;
	}

	res = open_freezer_files();
	if (res)
		return res;

	freezer_enabled = 1;
	LOGI("Freezer enabled\n");

	return res;
}

int destroy_freezer_subgroup(void)
{
	struct stat st;
	int res = 0;

	freezer_enabled = 0;
	LOGI("Freezer disabled\n");

	res = thaw_subgroup();
	if (res)
		return res;

	res = close_freezer_files();
	if (res)
		return res;

	if (stat(FREEZER_SUBGROUP, &st) != -1)
		res = rmdir(FREEZER_SUBGROUP);

	return res;
}

static int add_tasks_to_freezer(void)
{
	ssize_t res;
	pid_t pid;

	if (!freezer_enabled)
		return 0;

	if (f_tasks_fd == NULL) {
		LOGE("Tasks freezer subgroup file is closed\n");
		return -1;
	}

	if (f_us_man_fd == NULL) {
		LOGE("Us_manager tasks file is closed\n");
		return -1;
	}

	while(fscanf(f_us_man_fd, "%u", &pid) == 1) {
		res = fprintf(f_tasks_fd, "%u", pid);
		if (res < 0) {
			LOGE("Cannot add task %u to freezer group\n", pid);
			return -1;
		}
	}
	fflush(f_tasks_fd);

	return 0;
}

int freeze_subgroup(void)
{
	size_t len = strlen(freezer_frozen) + 1;
	ssize_t res;

	if (!freezer_enabled)
		return 0;

	if (f_state_fd == NULL) {
		LOGE("Freezer.state subgroup file is closed\n");
		return -1;
	}

	res = add_tasks_to_freezer();
	if (res) {
		LOGE("Cannot add tasks to freezer\n");
		return -1;
	}

	res = fwrite(freezer_frozen, len, 1, f_state_fd);
	if (res < 0) {
		LOGE("Cannot set FROZEN state\n");
		return -1;
	}
	fflush(f_state_fd);

	return 0;
}

int thaw_subgroup(void)
{
	size_t len = strlen(freezer_thawed) + 1;
	ssize_t res;

	if (!freezer_enabled)
		return 0;

	if (f_state_fd == NULL) {
		LOGE("Freezer.state subgroup file is closed\n");
		return -1;
	}

	res = fwrite(freezer_thawed, len, 1, f_state_fd);
	if (res < 0) {
		LOGE("Cannot set THAWED state\n");
		return -1;
	}
	fflush(f_state_fd);

	return 0;
}
