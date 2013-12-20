/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
n *
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

#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include "da_protocol.h"
#include "da_data.h"
#include "daemon.h"
#include "debug.h"

#define INPUT_ID_TOUCH		0
#define INPUT_ID_KEY		1
#define STR_TOUCH		"TOUCH"
#define STR_KEY			"KEY"
#define INPUT_ID_STR_KEY	"ID_INPUT_KEY=1"
#define INPUT_ID_STR_TOUCH	"ID_INPUT_TOUCHSCREEN=1"
#define INPUT_ID_STR_KEYBOARD	"ID_INPUT_KEYBOARD=1"
#define INPUT_ID_STR_TABLET	"ID_INPUT_TABLET=1"

#define MAX_FILENAME		128 /* TODO: remove */
#define BUF_SIZE		1024 /* TODO: remove */
#define MAX_DEVICE		10
#define ARRAY_END		(-11)

typedef struct _input_dev
{
	int fd;
	char fileName[MAX_FILENAME];
} input_dev;

static input_dev g_key_dev[MAX_DEVICE];
static input_dev g_touch_dev[MAX_DEVICE];

#define MAX_EVENTS_NUM 10
static int device_event_handler(input_dev *dev, int input_type)
{
	int ret = 0;
	ssize_t size = 0;
	int count = 0;
	struct input_event in_ev[MAX_EVENTS_NUM];
	struct msg_data_t *log;

	if (input_type == INPUT_ID_TOUCH || input_type == INPUT_ID_KEY) {
		do {
			size = read(dev->fd, &in_ev[count], sizeof(*in_ev));
			/* TODO: add error check */
			if (size > 0)
				count++;
		} while (count < MAX_EVENTS_NUM && size > 0);

		if (count) {
			LOGI("read %d %s events\n",
			     count,
			     input_type == INPUT_ID_KEY ? STR_KEY : STR_TOUCH);
			log = gen_message_event(in_ev, count, input_type);
			printBuf((char *)log, MSG_DATA_HDR_LEN + log->len);
			write_to_buf(log);
			free_msg_data(log);
		}
	} else {
		LOGE("unknown input_type\n");
		ret = -1;
	}
	return ret;
}

static int process_input_event(int fd)
{
	int k;

	for (k = 0; g_touch_dev[k].fd != ARRAY_END; k++) {
		if (g_touch_dev[k].fd > 0 &&
		    fd == g_touch_dev[k].fd) {
			if (device_event_handler(&g_touch_dev[k],
						 INPUT_ID_TOUCH))
				LOGE("Cannot process touch event\n");
			break;
		}
	}

	for (k = 0; g_key_dev[k].fd != ARRAY_END; k++) {
		if (g_key_dev[k].fd > 0 &&
		    fd == g_key_dev[k].fd) {
			if (device_event_handler(&g_key_dev[k], INPUT_ID_KEY))
				LOGE("Cannot process touch event\n");
			break;
		}
	}

	return 0;
}

void register_input_events(void)
{
	int i;

	for (i = 0; g_key_dev[i].fd != ARRAY_END; i++) {
		if (g_key_dev[i].fd > 0) {
			if (register_event_handler(g_key_dev[i].fd,
						   process_input_event))
				LOGW("Cannot register key event\n");
		}
	}

	for (i = 0; g_touch_dev[i].fd != ARRAY_END; i++) {
		if (g_touch_dev[i].fd > 0) {
			if (register_event_handler(g_touch_dev[i].fd,
						   process_input_event))
				LOGW("Cannot register touch event\n");
		}
	}
}

void unregister_input_events(void)
{
	int i;

	for (i = 0; g_key_dev[i].fd != ARRAY_END; i++) {
		if (g_key_dev[i].fd > 0) {
			if (unregister_event_handler(g_key_dev[i].fd))
				LOGW("Cannot unregister key event\n");
		}
	}

	for (i = 0; g_touch_dev[i].fd != ARRAY_END; i++) {
		if (g_touch_dev[i].fd > 0) {
			if (unregister_event_handler(g_touch_dev[i].fd))
				LOGW("Cannot unregister touch event\n");
		}
	}
}

// return bytes size of readed data
// return 0 if no data readed or error occurred
static int _file_read(FILE *fp, char *buffer, int size)
{
	int ret = 0;

	if (fp != NULL && size > 0) {
		ret = fread((void *)buffer, sizeof(char), size, fp);
		buffer[ret] = '\0';
	} else {
		// fp is null
		if (size > 0)
			buffer[0] = '\0';
		ret = 0;	// error case
	}

	return ret;
}

// get input id of given input device
static int get_input_id(char *inputname)
{
	static int query_cmd_type = 0;	// 1 if /lib/udev/input_id, 2 if udevadm
	FILE *cmd_fp = NULL;
	char buffer[BUF_SIZE];
	char command[MAX_FILENAME];
	int ret = -1;

	// determine input_id query command
	if (unlikely(query_cmd_type == 0)) {
		if (access("/lib/udev/input_id", F_OK) == 0) {
			// there is /lib/udev/input_id
			query_cmd_type = 1;
		} else {
			// there is not /lib/udev/input_id
			query_cmd_type = 2;
		}
	}
	// make command string
	if (query_cmd_type == 1) {
		sprintf(command, "/lib/udev/input_id /class/input/%s",
			inputname);
	} else {
		sprintf(command,
			"udevadm info --name=input/%s --query=property",
			inputname);
	}

	// run command
	cmd_fp = popen(command, "r");
	if (_file_read(cmd_fp, buffer, BUF_SIZE) < 0) {
		LOGE("Failed to read input_id\n");
		if (cmd_fp != NULL)
			pclose(cmd_fp);
		return ret;
	}
	// determine input id
	if (strstr(buffer, INPUT_ID_STR_KEY)) {
		// key
		ret = INPUT_ID_KEY;
	} else if (strstr(buffer, INPUT_ID_STR_TOUCH)) {
		// touch
		ret = INPUT_ID_TOUCH;
	} else if (strstr(buffer, INPUT_ID_STR_KEYBOARD)) {
		// keyboard
		ret = INPUT_ID_KEY;
	} else if (strstr(buffer, INPUT_ID_STR_TABLET)) {
		// touch (emulator)
		ret = INPUT_ID_TOUCH;
	}

	if (cmd_fp != NULL)
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

	if (dp != NULL) {
		while ((d = readdir(dp)) != NULL) {
			if (!strncmp(d->d_name, "event", 5)) {
				if (input_id == get_input_id(d->d_name)) {
					sprintf(dev[count].fileName,
						"/dev/input/%s", d->d_name);
					dev[count].fd =
					    open(dev[count].fileName,
						 O_RDWR | O_NONBLOCK);
					count++;
				}
			}
		}

		closedir(dp);
	}
	dev[count].fd = ARRAY_END;
}

void init_input(void)
{
	_get_fds(g_key_dev, INPUT_ID_KEY);
	_get_fds(g_touch_dev, INPUT_ID_TOUCH);
}

static void _device_write(input_dev *dev, const struct input_event *in_ev)
{
	int i;
	for (i = 0; dev[i].fd != ARRAY_END; i++) {
		if (dev[i].fd >= 0) {
			write(dev[i].fd, in_ev, sizeof(struct input_event));
			LOGI("write(%d, %d, %d)\n",
			     dev[i].fd, (int)in_ev, sizeof(struct input_event));
		}
	}
}

void write_input_event(int id, const struct input_event *ev)
{
	switch (id) {
	case INPUT_ID_TOUCH:
		LOGI_th_rep("event -> %s\n", INPUT_ID_STR_TOUCH);
		_device_write(g_touch_dev, ev);
		break;

	case INPUT_ID_KEY:
		LOGI_th_rep("event -> %s\n", INPUT_ID_STR_KEY);
		_device_write(g_key_dev, ev);
		break;
	default:
		LOGE("event -> UNKNOWN INPUT ID");
	}
}
