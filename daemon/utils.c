/*
*  DA manager
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: 
*
* Jaewon Lim <jaewon81.lim@samsung.com>
* Woojin Jung <woojin2.jung@samsung.com>
* Juyoung Kim <j0.kim@samsung.com>
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
*
*/ 

#include <stdlib.h>
#include <pthread.h>
#include "utils.h"
#include "da_debug.h"

#define BUFFER_MAX		1024

static int file_fd = -1;

static pthread_mutex_t write_mutex;

// get application name from executable binary path
char* get_app_name(char* binary_path)
{
	char* pos;

	pos = strrchr(binary_path, '/');
	if(pos != NULL)
		pos++;

	return pos;
}

// execute applcation with executable binary path
// return 0 to fail to execute
// return 1 to succeed to execute
int exec_app(const char* exec_path, int app_type)
{
	pid_t   pid;
	char command[PATH_MAX];

	if (exec_path == NULL || strlen(exec_path) <= 0) 
	{
		LOGE("Executable path is not correct\n");
		return 0;
	}

	if (( pid = fork()) < 0)	// fork error
    	return 0;

    else if(pid > 0)
    	return 1;		// exit parent process with successness

	sprintf(command, "%s %s", DA_PRELOAD(app_type), exec_path);
	LOGI("launch app path is %s, executable path is %s\n", LAUNCH_APP_PATH, exec_path);
	execl(SHELL_CMD, SHELL_CMD, "-c", command, NULL);

	return 1;
}

#ifdef USE_LAUNCH_PAD
// execute application by launch pad
// return 0 to fail to execute
// return 1 to succeed to execute
int exec_app_by_launchpad(const char* pkg_name)
{
	pid_t   pid;

	if (pkg_name == NULL || strlen(pkg_name) <= 0) 
	{
		LOGE("Package name is not correct\n");
		return 0;
	}

	if (( pid = fork()) < 0)	// fork error
    	return 0;

    else if(pid > 0)
    	return 1;		// exit parent process with successness

	LOGI("launch app path is %s, pkg name is %s\n", LAUNCH_APP_PATH, pkg_name);
	execl(LAUNCH_APP_PATH, LAUNCH_APP_NAME, pkg_name, DA_PRELOAD_EXEC, NULL);

	return 1;
}
#endif

#if 0
void kill_app(const char* binary_path)		// fork version
{
	pid_t pid;
	pid_t pkg_pid;
	char command[PATH_MAX];

	if (( pid = fork()) < 0)
    	return;

    //exit parent process
    else if(pid > 0)
    	return;
	
	pkg_pid = find_pid_from_path(binary_path);

	if(pkg_pid > 0)
	{
		sprintf(command, "kill -9 %d", pkg_pid);
		LOGI(" : %s\n", command);
		execl(SHELL_CMD, SHELL_CMD, "-c", command, NULL);
	}
	else
	{
		exit(0);
	}
}
#else
void kill_app(const char* binary_path)		// non fork version
{
	pid_t pkg_pid;
	char command[PATH_MAX];

	pkg_pid = find_pid_from_path(binary_path);

	if(pkg_pid > 0)
	{
		sprintf(command, "kill -9 %d", pkg_pid);
		LOGI("kill app : %s\n", command);
		system(command);
	}
}
#endif

// find process id from executable binary path
pid_t find_pid_from_path(const char* path)
{
	pid_t status = -1;

	char buffer [BUFFER_MAX];
	char command [BUFFER_MAX];

	sprintf(command, "/bin/pidof %s", path);

	FILE *fp = popen(command, "r");
	if (!fp)
	{
		LOGE("Getting pidof %s is failed\n", path);
		return status;
	}

	while (fgets(buffer, BUFFER_MAX, fp) != NULL)
	{
		LOGI("result of 'pidof' is %s\n", buffer);
	}

	pclose(fp);

	if (strlen(buffer) > 0)
	{
		if (sscanf(buffer,"%d\n", &status) != 1)
		{
			LOGE("Failed to read result buffer of 'pidof', status(%d)\n", status);
			return -1;
		}
	}

	return status;
}

static void mkdirs()
{
	mkdir("/home/developer/sdk_tools/da", 0775);
	mkdir("/home/developer/sdk_tools/da/battery", 0775);
}

int create_open_batt_log(const char* app_name) 
{
	char log_path[PATH_MAX];

	pthread_mutex_init(&write_mutex, NULL);
	sprintf(log_path, "%s%s", BATT_LOG_FILE, app_name);

	if ((file_fd = open(log_path, O_WRONLY|O_CREAT|O_TRUNC)) == -1)
	{
		mkdirs();
		if ((file_fd = open(log_path, O_WRONLY|O_CREAT|O_TRUNC)) == -1)
		{		
			LOGE("Failed to open batt log file\n");
			return 0;
		}
	}

	return file_fd;
}

int get_batt_fd()
{
	return file_fd;
}

int write_batt_log(const char* msg) 
{
	int length = -1;

	if (file_fd == -1 ) 
	{
		return -1;
	}

	pthread_mutex_lock(&write_mutex);

	length = write(file_fd, msg, strlen(msg));

	pthread_mutex_unlock(&write_mutex);

	return length;
}

void close_batt_fd()
{
	close(file_fd);
	file_fd = -1;
}

#if DEBUG
void write_log()
{
    int fd;

    fd = open("/dev/null", O_RDONLY);
    dup2(fd, 0);

    fd = open("/tmp/da_daemon.log", O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if(fd < 0) {
        fd = open("/dev/null", O_WRONLY);
    }
    dup2(fd, 1);
    dup2(fd, 2);

    fprintf(stderr,"--- da starting (pid %d) ---\n", getpid());
}
#endif


