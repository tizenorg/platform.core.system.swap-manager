/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim <jaewon81.lim@samsung.com>
 * Woojin Jung <woojin2.jung@samsung.com>
 * Juyoung Kim <j0.kim@samsung.com>
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
 * Nikita Kalyazin    <n.kalyazin@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Contributors:
 * - S-Core Co., Ltd
 * - Samsung RnD Institute Russia
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <signal.h>			// for signal
#include <pthread.h>
#include <unistd.h>		// for unlink
#include <dirent.h>		// for opendir, readdir
#include <sys/types.h>	// for open
#include <sys/stat.h>	// for open
#include <fcntl.h>		// for open
#include <grp.h>		// for setgroups
#include <sys/wait.h> /* waitpid */

#include "daemon.h"
#include "utils.h"
#include "smack.h"
#include "swap_debug.h"

#define BUFFER_MAX			1024
#define SID_APP				5000
#define MANIFEST_PATH		"/info/manifest.xml"

#define PROC_FS			"/proc"
#define PROC_CMDLINE		"/proc/%s/cmdline"

#define APPDIR1				"/opt/apps/"
#define APPDIR2				"/opt/usr/apps/"

uint64_t str_to_uint64(char* str)
{
	uint64_t res = 0;

	if(str != NULL)
	{
		while(*str >= '0' && *str <= '9')
		{
			res = res * 10 + (*str - '0');
			str++;
		}
	}

	return res;
}

int64_t str_to_int64(char* str)
{
	int64_t res = 0;
	int64_t factor = 1;

	if(str != NULL)
	{
		if(*str == '-')
		{
			factor = -1;
			str++;
		}
		else if(*str == '+')
		{
			factor = 1;
			str++;
		}

		while(*str >= '0' && *str <= '9')
		{
			res = res * 10 + (*str - '0');
			str++;
		}
	}

	return (res * factor);
}

// return 0 if succeed
// return -1 if error occured
int remove_indir(const char *dirname)
{
	DIR *dir;
	struct dirent *entry;
	static char dirent_buffer[ sizeof(struct dirent) + PATH_MAX + 1 ] = {0,};
	static struct dirent *dirent_r = (struct dirent *)dirent_buffer;
	char path[PATH_MAX];

	dir = opendir(dirname);
	if(dir == NULL)
	{
		return -1;
	}

	while ((readdir_r(dir, dirent_r, &entry) == 0) && entry) {
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
		{
			snprintf(path, (size_t) PATH_MAX, "%s/%s", dirname, entry->d_name);
			if (entry->d_type != DT_DIR)	// file
			{
				unlink(path);
			}
			else { }	// directory
		}
	}
	closedir(dir);

	return 0;
}

/* execute applcation with executable binary path */
int exec_app_tizen(const char *app_id, const char *exec_path)
{
	pid_t pid;

	LOGI("exec %s\n", exec_path);

	if (exec_path == NULL || !strlen(exec_path)) {
		LOGE("Executable path is not correct\n");
		return -1;
	}
	LOGI("launch app path is %s, executable path is %s\n"
	     "launch app name (%s), app_id (%s)\n",
	     LAUNCH_APP_PATH, exec_path, LAUNCH_APP_NAME, app_id);
	pid = fork();
	if (pid == -1)
		return -1;

	if (pid > 0) { /* parent */
		int status, ret;
		do
			ret = waitpid(pid, &status, 0);
		while (ret == -1 && errno == EINTR);
		return 0;
	} else { /* child */
		execl(LAUNCH_APP_PATH, LAUNCH_APP_NAME, app_id, NULL);
		/* FIXME: If code flows here, it deserves greater attention */
		_Exit(EXIT_FAILURE);
	}
}

int exec_app_common(const char* exec_path)
{
	pid_t pid;
	char command[PATH_MAX];

	LOGI("exec %s\n", exec_path);
	if (exec_path == NULL || !strlen(exec_path)) {
		LOGE("Executable path is not correct\n");
		return -1;
	}

	snprintf(command, sizeof(command), "%s", exec_path);
	LOGI("cmd: <%s>\n", command);

	pid = fork();
	if (pid == -1)
		return -1;

	if (pid > 0) { /* parent */
		return 0;
	} else { /* child */
		execl(SHELL_CMD, SHELL_CMD, "-c", command, NULL);
		/* FIXME: Again, such case deserve much more attention */
		_Exit(EXIT_FAILURE);
	}
}

int exec_app_web(const char *app_id)
{
	pid_t pid;

	LOGI("wrt-launcher path is %s,\n"
	     "wrt-launcher name (%s), app_id (%s)\n",
	     WRT_LAUNCHER_PATH, WRT_LAUNCHER_NAME, app_id);

	pid = fork();
	if (pid == -1)
		return -1;

	if (pid > 0) { /* parent */
		int status, ret;
		do
			ret = waitpid(pid, &status, 0);
		while (ret == -1 && errno == EINTR);
		return 0;
	} else { /* child */
		execl(WRT_LAUNCHER_PATH,
		      WRT_LAUNCHER_NAME,
		      (is_feature_enabled(FL_WEB_PROFILING) ?
		       WRT_LAUNCHER_START_DEBUG : WRT_LAUNCHER_START),
		      app_id,
		      NULL);
		/* FIXME: If code flows here, it deserves greater attention */
		LOGE("Cannot run exec!\n");
		_Exit(EXIT_FAILURE);
	}
}

void kill_app_web(const char *app_id)
{
	pid_t pid;

	LOGI("wrt-launcher path is %s,\n"
	     "wrt-launcher name (%s), app_id (%s)\n",
	     WRT_LAUNCHER_PATH, WRT_LAUNCHER_NAME, app_id);

	pid = fork();
	if (pid == -1)
		return;

	if (pid > 0) { /* parent */
		int status, ret;
		do
			ret = waitpid(pid, &status, 0);
		while (ret == -1 && errno == EINTR);
		return;
	} else { /* child */
		execl(WRT_LAUNCHER_PATH,
		      WRT_LAUNCHER_NAME,
		      WRT_LAUNCHER_KILL,
		      app_id,
		      NULL);
		/* FIXME: If code flows here, it deserves greater attention */
		LOGE("Cannot run exec!\n");
		_Exit(EXIT_FAILURE);
	}
}

// find process id from executable binary path
static pid_t find_pid_from_path(const char *path)
{
	char buf[BUFFER_MAX + 1];
	char cmdline[PATH_MAX];
	DIR *proc;
	FILE *fp = NULL;
	static const char scan_format[] = "%" STR_VALUE(BUFFER_MAX) "s";
	char dirent_buffer[ sizeof(struct dirent) + PATH_MAX + 1 ] = {0,};
	struct dirent *dirent_r = (struct dirent *)dirent_buffer;
	struct dirent *entry;
	int found, len = strlen(path);
	pid_t pid = 0;

	LOGI("look for <%s>\n", path);

	proc = opendir(PROC_FS);
	if (!proc) {
		LOGE("cannot open proc fs <%s>\n", PROC_FS);
		goto out;
	}

	while ((readdir_r(proc, dirent_r, &entry) == 0) && entry) {
		pid = (pid_t)atoi(entry->d_name);
		if (pid == 0)
			continue;

		snprintf(cmdline, sizeof(cmdline), PROC_CMDLINE, entry->d_name);

		fp = fopen(cmdline, "r");
		if (fp == NULL)
			continue;

		found = 0;
		if (fscanf(fp, scan_format, buf) != EOF) /* read only argv[0] */
			found = (strncmp(path, buf, len) == 0);

		fclose(fp);
		if (found)
			goto out_close_dir;
	}

	pid = 0;

out_close_dir:
	closedir(proc);

out:
	return pid;
}

static pid_t get_pid_by_path(const char *binary_path)
{
	pid_t pkg_pid;
	int len;
	char *real_path;
	static const char exe_line[] = ".exe";


	pkg_pid = find_pid_from_path(binary_path);
	if (pkg_pid == 0) {
		len = strlen(binary_path);
		real_path = malloc(len + sizeof(exe_line));
		if (real_path == NULL) {
			LOGE("cannot alloc memory\n");
			return -1;
		}
		memcpy(real_path, binary_path, len + 1);

		// try remove or add ".exe" and get pid
		if (strcmp(real_path + len - (sizeof(exe_line) - 1), exe_line) == 0)
			// remove .exe from tPath
			real_path[len - (sizeof(exe_line) - 1)] = '\0';
		else
			// add .exe
			memcpy(&real_path[len], exe_line, sizeof(exe_line));

		pkg_pid = find_pid_from_path(real_path);
		free(real_path);
	}

	return pkg_pid;
}

static int find_alternative_bin_path(const char *binary_path, char *alter_bin_path,
	   size_t  buflen)
{
	// alternative path may be /opt/apps/... or /opt/usr/apps/...)
	char *add_fname;
	if (strncmp(binary_path, APPDIR1, strlen(APPDIR1)) == 0) {
		strncpy(alter_bin_path, APPDIR2, buflen);
		buflen -= strlen(alter_bin_path);
		add_fname = (char *)binary_path + strlen(APPDIR1);
		strncat(alter_bin_path, add_fname, buflen);
	} else if (strncmp(binary_path, APPDIR2, strlen(APPDIR2)) == 0) {
		strncpy(alter_bin_path, APPDIR1, buflen);
		buflen -= strlen(alter_bin_path);
		add_fname = (char *)binary_path + strlen(APPDIR2);
		strncat(alter_bin_path, add_fname, buflen);
	} else {
		return 1;
	}
	return 0;
}

int kill_app(const char *binary_path)
{
#ifdef PROFILE_TV
	/* usage of SIGTERM signal isn't possible on TV */
	enum { FINISH_SIG = SIGKILL };
#else /* PROFILE_TV */
	enum { FINISH_SIG = SIGTERM };
#endif /* PROFILE_TV */

	pid_t pkg_pid;
	char alter_bin_path[PATH_MAX];

	LOGI("kill %s (%d)\n", binary_path, FINISH_SIG);

	pkg_pid = get_pid_by_path(binary_path);

	if (pkg_pid == 0) {
		if (find_alternative_bin_path(binary_path, alter_bin_path, PATH_MAX) == 0)
			pkg_pid = get_pid_by_path(alter_bin_path);
	}

	if (pkg_pid != 0) {
		if (kill(pkg_pid, FINISH_SIG) == -1) {
			GETSTRERROR(errno, err_buf);
			LOGE("cannot kill %d -%d errno<%s>\n", pkg_pid, FINISH_SIG,
			     err_buf);
			return -1;
		} else {
			// we need sleep up there because kill() function
			// returns control immediately after send signal
			// without it app_launch returns err on start app
			sleep(1);
			LOGI("killed %d -%d\n", pkg_pid, FINISH_SIG);
		}
	} else
		LOGI("cannot kill <%s>; process not found\n", binary_path);

	LOGI("kill< %s (%d)\n", binary_path, FINISH_SIG);
	return 0;
}

static char *dereference_tizen_exe_path(const char *path, char *resolved);

int is_same_app_process(char* appPath, int pid)
{
	int ret = 0;
	int tlen;
	FILE *fp;
	char buf[BUFFER_MAX];
	char cmdPath[PATH_MAX];
	char tPath[PATH_MAX];
	char buf_res[PATH_MAX];
	char tPath_res[PATH_MAX];

	strncpy(tPath, appPath, PATH_MAX - 1);
	tlen = strlen(tPath);
	if(strcmp(tPath + tlen - 4, ".exe") == 0)
	{
		// remove .exe from tPath
		tPath[tlen - 4] = '\0';
	}

	snprintf(cmdPath, sizeof(cmdPath), "/proc/%d/cmdline", pid);

	if((fp = fopen(cmdPath, "r")) == NULL)
	{
		return 0;
	}

	if(fgets(buf, BUFFER_MAX, fp) != NULL)
	{
		tlen = strlen(buf);
		if(strcmp(buf + tlen - 4, ".exe") == 0)
		{
			// remove .exe from tPath
			buf[tlen - 4] = '\0';
		}

		dereference_tizen_exe_path(buf, buf_res);
		dereference_tizen_exe_path(tPath, tPath_res);

		if(strcmp(buf_res, tPath_res) == 0)
			ret = 1;
		else
			ret = 0;
	}
	fclose(fp);

	return ret;
}

static char *dereference_tizen_exe_path(const char *path, char *resolved)
{
	char *res = NULL;
	char tmp_path[PATH_MAX];

	resolved[0] = 0;
	//try resolve <path>.exe
	snprintf(tmp_path, sizeof(tmp_path), "%s.exe", path);
	if ((res = realpath(tmp_path, resolved)) == NULL) {
		//try to resolve path <path>
		res = realpath(path, resolved);
	}

	return res;
}

float get_uptime(void)
{
	const char *LINUX_UPTIME_FILE = "/proc/uptime";
	FILE *fp = fopen(LINUX_UPTIME_FILE, "r");
	float uptime;
	if (!fp)
		return 0.0;

	if (fscanf(fp, "%f", &uptime) != 1)
		uptime = 0.0;

	fclose(fp);
	return uptime;
}

void swap_usleep(useconds_t usec)
{
	struct timespec req;
	struct timespec rem;
	req.tv_sec = usec / 1000000;
	req.tv_nsec = (usec % 1000000) * 1000;
	if (nanosleep(&req, &rem) == -1) {
		LOGW("sleep was terminated by signal\n");
	}
}
