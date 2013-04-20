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
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <signal.h>
#include <linux/input.h>

#define BUF_SIZE					1024	
#define INPUT_ID_STR_KEY			"ID_INPUT_KEY=1"
#define INPUT_ID_STR_TOUCH			"ID_INPUT_TOUCHSCREEN=1"
#define INPUT_ID_STR_KEYBOARD		"ID_INPUT_KEYBOARD=1"
#define INPUT_ID_STR_TABLET			"ID_INPUT_TABLET=1"
#define MAX_DEVICE					10
#define MAX_FILENAME				128

#define INPUT_ID_TOUCH			0
#define INPUT_ID_KEY			1	
#define STR_TOUCH				"TOUCH"
#define STR_KEY					"KEY"
#define ARRAY_END				(-11)

#define DAEMON_SENDER		0x01
#define DAEMON_RECEIVER		0x10

typedef struct _input_dev
{
	int fd;
	char fileName[MAX_FILENAME];
} input_dev;

// ====================================================================
// helper functions
// ====================================================================

// return bytes size of readed data
// return 0 if no data readed or error occurred
static int _file_read(FILE* fp, char *buffer, int size)
{
	int ret = 0;

	if(fp != NULL && size > 0)
	{
		ret = fread((void*)buffer, sizeof(char), size, fp);
	}
	else
	{
		// fp is null
		if(size > 0)
			buffer[0] = '\0';

		ret = 0;	// error case
	}

	return ret;
}

// get input id of given input device
static int get_input_id(char* inputname)
{
	static int query_cmd_type = 0;	// 1 if /lib/udev/input_id, 2 if udevadm
	FILE* cmd_fp = NULL;
	char buffer[BUF_SIZE];
	char command[MAX_FILENAME];
	int ret = -1;

	// determine input_id query command
	if(__builtin_expect(query_cmd_type == 0, 0))
	{
		sprintf(command, "ls /lib/udev/input_id; echo cmd_ret:$?;");
		cmd_fp = popen(command, "r");
		_file_read(cmd_fp, buffer, BUF_SIZE);
		if(strstr(buffer, "cmd_ret:0"))		// there is /lib/udev/input_id
		{
			query_cmd_type = 1;
		}
		else	// there is not /lib/udev/input_id
		{
			query_cmd_type = 2;
		}
		if(cmd_fp != NULL)
			pclose(cmd_fp);
	}

	// make command string
	if(query_cmd_type == 1)
	{
		sprintf(command, "/lib/udev/input_id /class/input/%s", inputname);
	}
	else if(query_cmd_type == 2)
	{
		sprintf(command, "udevadm info --name=input/%s --query=property", inputname);
	}
	else
	{
		// not impossible
		perror("query cmd type is not valid");
		exit(0);
	}

	// run command
	cmd_fp = popen(command, "r");
	_file_read(cmd_fp, buffer, BUF_SIZE);

	// determine input id
	if(strstr(buffer, INPUT_ID_STR_KEY))			// key
	{
		ret = INPUT_ID_KEY;
	}
	else if(strstr(buffer, INPUT_ID_STR_TOUCH))		// touch
	{
		ret = INPUT_ID_TOUCH;
	}
	else if(strstr(buffer, INPUT_ID_STR_KEYBOARD))	// keyboard
	{
		ret = INPUT_ID_KEY;
	}
	else if(strstr(buffer, INPUT_ID_STR_TABLET))	// touch (emulator)
	{
		ret = INPUT_ID_TOUCH;
	}

	if(cmd_fp != NULL)
		pclose(cmd_fp);
	return ret;
}

// get filename and fd of given input type devices
static void _get_fds(input_dev *dev, int input_id)
{
	DIR *dp;
	struct dirent *d;
	int count = 0;

	dp = opendir("/sys/class/input");

	if(dp != NULL)
	{
		while((d = readdir(dp)) != NULL)
		{
			if(!strncmp(d->d_name, "event", 5))	// start with "event"
			{
				// event file
				if(input_id == get_input_id(d->d_name))
				{
					sprintf(dev[count].fileName, "/dev/input/%s", d->d_name);
					dev[count].fd = open(dev[count].fileName, O_RDWR);
					count++;
				}
			}
		}

		closedir(dp);
	}
	dev[count].fd = ARRAY_END;	// end of input_dev array
}

// ================================================================
// device write function
// ================================================================

static void _device_write(input_dev *dev, struct input_event* in_ev)
{
	int i;
	for(i = 0; dev[i].fd != ARRAY_END; i++)
	{
		if(dev[i].fd >= 0)
			write(dev[i].fd, in_ev, sizeof(struct input_event));
	}
}

// =================================================================
// file descriptor manipulation functions
// =================================================================

static void _fd_insert(fd_set* fdset, input_dev *dev)
{
	int i;
	for(i = 0; dev[i].fd != ARRAY_END; i++)
	{
		if(dev[i].fd >= 0)
		{
			FD_SET(dev[i].fd, fdset);
		}
	}
}

static int _fd_isset(fd_set* fdset, input_dev *dev)
{
	int i;
	for(i = 0; dev[i].fd != ARRAY_END; i++)
	{
		if(dev[i].fd >= 0)
		{
			if(FD_ISSET(dev[i].fd, fdset))
			{
				return i;
			}
		}
	}

	return -1;
}


int _get_maxfd(input_dev *touch, input_dev *key)
{
	int i;
	int maxfd = -1;

	for(i = 0 ; touch[i].fd != ARRAY_END; i++)
	{
		if(touch[i].fd > maxfd)
			maxfd = touch[i].fd;
	}

	for(i = 0 ; key[i].fd != ARRAY_END; i++)
	{
		if(key[i].fd > maxfd)
			maxfd = key[i].fd;
	}

	return maxfd;
}

// ============================================================================
// sender and receiver functions
// ============================================================================

static int sender()
{
	struct input_event in_ev;
	char eventType[MAX_FILENAME];

	input_dev key_dev[MAX_DEVICE];
	input_dev touch_dev[MAX_DEVICE];

	_get_fds(key_dev, INPUT_ID_KEY);
	_get_fds(touch_dev, INPUT_ID_TOUCH);

	while(1)
	{
		scanf("%s %hx %hx %d", eventType, &in_ev.type, &in_ev.code, &in_ev.value);
		if(!strncmp(eventType, STR_TOUCH, strlen(STR_TOUCH)))
		{
			_device_write(touch_dev, &in_ev);
		}
		else if(!strncmp(eventType, STR_KEY, strlen(STR_KEY)))
		{
			_device_write(key_dev, &in_ev);
		}
	}

	return 0;
}

static int receiver()
{
	fd_set allfds, readfds;
	int maxfd;
	int index;
	struct input_event in_ev;

	input_dev key_dev[MAX_DEVICE];
	input_dev touch_dev[MAX_DEVICE];

	_get_fds(key_dev, INPUT_ID_KEY);
	_get_fds(touch_dev, INPUT_ID_TOUCH);

	FD_ZERO(&readfds);
	_fd_insert(&readfds, key_dev);
	_fd_insert(&readfds, touch_dev);

	maxfd = _get_maxfd(touch_dev, key_dev);

	while(1)
	{
		allfds = readfds;

		select(maxfd + 1, &allfds, NULL, NULL, NULL);

		if((index = _fd_isset(&allfds, touch_dev)) != -1)
		{
			//touch read
			read(touch_dev[index].fd, &in_ev, sizeof(struct input_event));
			printf("%s %s %ld %ld %x %x %d\n",
					STR_TOUCH, touch_dev[index].fileName, in_ev.time.tv_sec,
					in_ev.time.tv_usec, in_ev.type, in_ev.code, in_ev.value);
		}
		else if ((index = _fd_isset(&allfds, key_dev)) != -1)
		{
			//key read
			read(key_dev[index].fd, &in_ev, sizeof(struct input_event));
			printf("%s %s %ld %ld %x %x %d\n",
					STR_KEY, key_dev[index].fileName, in_ev.time.tv_sec,
					in_ev.time.tv_usec, in_ev.type, in_ev.code, in_ev.value);
		}
	}

	return 0;
}

// ======================================================================
// main function and end signal handler
// ======================================================================

static void end_program(int sig)
{
	exit(0);
}

int main(int argc, char **argv)
{
	int opt;
	unsigned int commandType = 0;
	signal(SIGINT, end_program);

	while((opt = getopt(argc, argv, "sr")) != -1)
	{
		switch(opt)
		{
			case 's':		// send
				commandType |= DAEMON_SENDER;
				break;
			case 'r':		// receive
				commandType |= DAEMON_RECEIVER;
				break;
			default:		// unknown option
				commandType = 0;
				break;
		}

		if(commandType == 0)
			break;
	}

	if(commandType == DAEMON_SENDER)
	{
		sender();
	}
	else if(commandType == DAEMON_RECEIVER)
	{
		receiver();
	}
	else
	{
		// TODO : print help text
	}

	return 0;
}

