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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <unistd.h>		// for unlink
#include <dirent.h>		// for opendir, readdir
#include <sys/types.h>	// for open
#include <sys/stat.h>	// for open
#include <fcntl.h>		// for open
#include <grp.h>		// for setgroups
#ifndef LOCALTEST
#include <sys/smack.h>
#endif
#include "daemon.h"
#include "utils.h"
#include "debug.h"

#define BUFFER_MAX			1024
#define APP_GROUPS_MAX		100
#define APP_GROUP_LIST		"/usr/share/privilege-control/app_group_list"
#define SELF_LABEL_FILE		"/proc/self/attr/current"

#ifndef LOCALTEST
#define SMACK_LABEL_LEN		255
#endif

#define SID_APP				5000
#define MANIFEST_PATH		"/info/manifest.xml"

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

int read_line(const int fd, char* ptr, const unsigned int maxlen)
{
	unsigned int n = 0;
	char c[2];
	int rc;

	while(n != maxlen)
	{
		if((rc = read(fd, c, 1)) != 1)
			return -1; // eof or read err

		if(*c == '\n')
		{
			ptr[n] = 0;
			return n;
		}
		ptr[n++] = *c;
	}
	return -1; // no space
}

#ifndef LOCALTEST
int smack_set_label_for_self(const char *label)
{
	int len;
	int fd;
	int ret;

	len = strnlen(label, SMACK_LABEL_LEN + 1);
	if (len > SMACK_LABEL_LEN)
		return -1;

	fd = open(SELF_LABEL_FILE, O_WRONLY);
	if (fd < 0)
		return -1;

	ret = write(fd, label, len);
	close(fd);

	return (ret < 0) ? -1 : 0;
}
#endif

void set_appuser_groups(void)
{
	int fd = 0;
	char buffer[5];
	gid_t t_gid = -1;
	gid_t groups[APP_GROUPS_MAX] = {0, };
	int cnt = 0;

	// groups[cnt++] = SID_DEVELOPER;
	fd = open(APP_GROUP_LIST, O_RDONLY);
	if(fd < 0)
	{
		LOGE("cannot get app's group lists from %s", APP_GROUP_LIST);
		return;
	}

	for(;;)
	{
		if(read_line(fd, buffer, sizeof buffer) < 0)
		{
			break;
		}

		t_gid = strtoul(buffer, 0, 10);
		errno = 0;
		if(errno != 0)
		{
			LOGE("cannot change string to integer: [%s]\n", buffer);
			continue;
		}

		if(t_gid)
		{
			if(cnt < APP_GROUPS_MAX)
			{
				groups[cnt++] = t_gid;
			}
			else
			{
				LOGE("cannot add groups more than %d", APP_GROUPS_MAX);
				break;
			}
		}
	}

	if(cnt > 0)
	{
		if(setgroups(sizeof(groups) / sizeof(groups[0]), groups) != 0)
		{
			fprintf(stderr, "set groups failed errno: %d\n", errno);
			close(fd);
			exit(1);
		}
	}
	close(fd);
}

#ifndef LOCALTEST
int get_smack_label(const char* execpath, char* buffer, int buflen)
{
	char* appid = NULL;
	int rc;

	rc = smack_lgetlabel(execpath, &appid, SMACK_LABEL_ACCESS);
	if(rc == 0 && appid != NULL)
	{
		strncpy(buffer, appid, (buflen < strlen(appid))? buflen:strlen(appid));
		free(appid);
		return 0;
	}
	else
		return -1;
}
#endif

int get_appid(const char* execpath, char* buffer, int buflen)
{
	int i, ret = 0;
	char* temp;
	if(strncmp(execpath, APPDIR1, strlen(APPDIR1)) == 0)
	{
		execpath = execpath + strlen(APPDIR1);
		temp = strchr(execpath, '/');
		for(i = 0; i < strlen(execpath); i++)
		{
			if(execpath + i == temp)
				break;
			buffer[i] = execpath[i];
		}
		buffer[i] = '.';
		buffer[i+1] = '\0';
		temp = strrchr(execpath, '/');
		if(temp != NULL)
		{
			temp++;
			strcat(buffer, temp);
		}
	}
	else if(strncmp(execpath, APPDIR2, strlen(APPDIR2)) == 0)
	{
		execpath = execpath + strlen(APPDIR2);
		temp = strchr(execpath, '/');
		for(i = 0; i < strlen(execpath); i++)
		{
			if(execpath + i == temp)
				break;
			buffer[i] = execpath[i];
		}
		buffer[i] = '.';
		buffer[i+1] = '\0';
		temp = strrchr(execpath, '/');
		if(temp != NULL)
		{
			temp++;
			strcat(buffer, temp);
		}
	}
	else
	{
		ret = -1;
	}

	return ret;
}

// return 0 if succeed
// return -1 if error occured
int remove_indir(const char *dirname)
{
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];

	dir = opendir(dirname);
	if(dir == NULL)
	{
		return -1;
	}

	while((entry = readdir(dir)) != NULL)
	{
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

// get application name from executable binary path
char* get_app_name(char* binary_path)
{
	char* pos;

	pos = strrchr(binary_path, '/');
	if(pos != NULL)
		pos++;

	return pos;
}

// return 0 to normal case
// return non-zero to error case
int get_manifest_path(const char* exec_path, char* buf, int buflen)
{
	char* chr;

	strcpy(buf, exec_path);

	chr = strrchr(buf, '/');
	if(chr == NULL)
		return -1;

	*chr = '\0';

	chr = strrchr(buf, '/');
	if(chr == NULL)
		return -1;

	*chr = '\0';

	strcat(buf, MANIFEST_PATH);
	return 0;
}

// execute applcation with executable binary path
// return 0 to fail to execute
// return 1 to succeed to execute
int exec_app_tizen(const char *app_id, const char *exec_path)
{
	LOGI("exec %s\n", exec_path);
	pid_t pid;
	//char command[PATH_MAX];
	//char appid[PATH_MAX];

	if (exec_path == NULL || !strlen(exec_path)) {
		LOGE("Executable path is not correct\n");
		return 0;
	}

	pid = fork();
	if (pid == -1)
		return 1;
	else if (pid > 0)
		return 0; // exit parent process with success

	LOGI("launch app path is %s, executable path is %s\n",
	     LAUNCH_APP_PATH, exec_path);
	execl(LAUNCH_APP_PATH, LAUNCH_APP_NAME, app_id, LAUNCH_APP_SDK,
	      DA_PRELOAD_EXEC, NULL);

	return 0;
}

int exec_app_common(const char* exec_path)
{
	LOGI("kill %s\n", exec_path);

	pid_t pid;
	int hw_acc = 0;
	char manifest[PATH_MAX];
	char command[PATH_MAX];
#ifndef LOCALTEST
	// TODO: check if this makes sense
	/* char appid[SMACK_LABEL_LEN]; */
#endif
	if (exec_path == NULL || !strlen(exec_path))  {
		LOGE("Executable path is not correct\n");
		return 1;
	}

	pid = fork();
	if (pid == -1)
		return 1;
	else if (pid > 0)
		return 0; // exit parent process with success

	if (get_manifest_path(exec_path, manifest, PATH_MAX) == 0) {
		FILE* fp;
		char buffer[BUFFER_MAX];
		char* res;

		// grep for manifest
		sprintf(command, "grep \"HwAcceleration=\\\"On\\\"\" %s",
			manifest);
		fp = popen(command, "r");
		if (fp != NULL) {
			buffer[0] = '\0';
			res = fgets(buffer, BUFFER_MAX, fp);
			if (res != NULL && strlen(buffer) != 0) {
				hw_acc = 1;
			}
			pclose(fp);
		}
	}

	// FIXME: I think the followin does not make sense
/* #ifndef LOCALTEST */
/* 	if (get_smack_label(exec_path, appid, SMACK_LABEL_LEN - 1) < 0) { */
/* 		LOGE("failed to get smack label\n"); */
/* 		return 1; */
/* 	} else */
/* #endif */
/* 	{ */
/* #ifndef LOCALTEST */
/* 		LOGI("smack lable is %s\n", appid); */
/* 		if(smack_set_label_for_self(appid) < 0) { */
/* 			LOGE("failed to set label for self\n"); */
/* 		} */
/* #endif */
/* 		set_appuser_groups(); */
/* 		if (setgid(SID_APP) < 0) { */
/* 			LOGE("failed to setgid\n"); */
/* 		} */
/* 		if (setuid(SID_APP) < 0) { */
/* 			LOGE("failed to setuid\n"); */
/* 		} */

/* 		pid = getpid(); */
/* 		if (setpgid(pid, pid) < 0) { */
/* 			LOGE("failed to setpgid\n"); */
/* 		} */

/* 		if (hw_acc != 0) { */
/* 			sprintf(command, "HWACC=USE %s %s", DA_PRELOAD(app_type), exec_path); */
/* 		} else { */
/* 			sprintf(command, "%s %s", DA_PRELOAD(app_type), exec_path); */
/* 		} */

/* 		LOGI("launch app path is %s, executable path is %s\n", LAUNCH_APP_PATH, exec_path); */
/* 		execl(SHELL_CMD, SHELL_CMD, "-c", command, NULL); */
/* 		return 0; */
/* 	} */
	// TODO:
	/* if (hw_acc) */
	/* 	sprintf(command, "HWACC=USE %s %s", */
	/* 		DA_PRELOAD(app_type), exec_path); */
	/* else */
	/* 	sprintf(command, "%s %s", */
	/* 		DA_PRELOAD(app_type), exec_path); */
#ifndef LOCALTEST
	sprintf(command, "%s %s",
		DA_PRELOAD_OSP, exec_path);
#else /* LOCALTEST */
	sprintf(command, "%s", exec_path);
#endif /* LOCALTEST */

	LOGI("cmd: %s\n", command);
	execl(SHELL_CMD, SHELL_CMD, "-c", command, NULL);

	return 0;
}

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

void kill_app(const char* binary_path)
{
	LOGI("kill %s\n", binary_path);

	pid_t pkg_pid;
	char command[PATH_MAX];

	pkg_pid = find_pid_from_path(binary_path);
	if(pkg_pid > 0)
	{
		sprintf(command, "kill -9 %d", pkg_pid);
		system(command);
	}
}

int get_executable(char* appPath, char* buf, int buflen)
{
	int fd;

	sprintf(buf, "%s.exe", appPath);
	fd = open(buf, O_RDONLY);
	if(fd != -1)
	{
		close(fd);
	}
	else
	{
		strcpy(buf, appPath);
	}
	return 0;
}

int get_app_install_path(char *strAppInstall, int length)
{
	FILE *fp;
	char buf[BUFFER_MAX];
	char *p;
	int i;

	if ((fp = fopen(DA_INSTALL_PATH, "r")) == NULL)
	{
		LOGE("Failed to open %s\n", DA_INSTALL_PATH);
		return -1;
	}

	/*ex : <15>   DW_AT_comp_dir    : (indirect string, offset: 0x25f): /home/yt/workspace/templatetest/Debug-Tizen-Emulator	*/
	while (fgets(buf, BUFFER_MAX, fp) != NULL)
	{
		//name
		p = buf;
		for (i = 0; i < BUFFER_MAX; i++)
		{
			if (*p == ':')
				break;
			p++;
		}

		if (*p != ':')
			break;
		else
			p++;

		//(...,offset:...)
		for (; i < BUFFER_MAX; i++)
		{
			if (*p == '(')
			{
				while (*p != ')')
				{
					p++;
				}
			}
			if (*p == ':')
				break;
			p++;
		}

		//find
		if (*p != ':')
			break;
		for (; i < BUFFER_MAX; i++)
		{
			if (*p == ':' || *p == ' ' || *p == '\t')
				p++;
			else
				break;
		}

		//name
		if (strlen(p) <= length)
		{
			sprintf(strAppInstall, "%s", p);
			for (i = 0; i < strlen(p); i++)
			{
				if (strAppInstall[i] == '\n' || strAppInstall[i] == '\t')
				{
					strAppInstall[i] = '\0';
					break;
				}
			}
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return -1;
}

int is_app_built_pie(void)
{
	int result;
	FILE *fp;
	char buf[BUFFER_MAX];

	if((fp = fopen(DA_BUILD_OPTION, "r")) == NULL)
	{
		LOGE("Failed to open %s\n", DA_BUILD_OPTION);
		return -1;
	}

	if(fgets(buf, BUFFER_MAX, fp) != NULL)
	{
		if(strcmp(buf, "DYN\n") == 0)
			result = 1;
		else if(strcmp(buf, "EXEC\n") == 0)
			result = 0;
		else
			result = -1;
	}
	else
	{
		result = -1;
	}
	fclose(fp);

	return result;
}

int get_app_base_address(int *baseAddress)
{
	int res;
	FILE *fp;
	char buf[BUFFER_MAX];

	if((fp = fopen(DA_BASE_ADDRESS, "r")) == NULL)
	{
		LOGE("Failed to open %s\n", DA_BASE_ADDRESS);
		return -1;
	}

	if(fgets(buf, BUFFER_MAX, fp) != NULL)
	{
		res = sscanf(buf, "%x", baseAddress);
	}
	else
	{
		res = -1;
	}
	fclose(fp);

	return res;
}

int is_same_app_process(char* appPath, int pid)
{
	int ret = 0;
	FILE *fp;
	char buf[BUFFER_MAX];
	char cmdPath[PATH_MAX];
	char execPath[PATH_MAX];

	get_executable(appPath, execPath, PATH_MAX);
	sprintf(cmdPath, "/proc/%d/cmdline", pid);

	if((fp = fopen(cmdPath, "r")) == NULL)
	{
		return 0;
	}

	if(fgets(buf, BUFFER_MAX, fp) != NULL)
	{
#ifdef RUN_APP_LOADER
		if(strcmp(buf, appPath) == 0)
#else
		// use execPath instead of manager.appPath
		if(strcmp(buf, execPath) == 0)
#endif
			ret = 1;
		else
			ret = 0;
	}
	fclose(fp);

	return ret;
}

