/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim		<jaewon81.lim@samsung.com>
 * Woojin Jung		<woojin2.jung@samsung.com>
 * Juyoung Kim		<j0.kim@samsung.com>
 * Cherepanov Vitaliy	<v.cherepanov@samsung.com>
 * Nikita Kalyazin	<n.kalyazin@samsung.com>
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


#include <dirent.h>
#include <Ecore.h>

#include "debug.h"
#include "da_data.h"
#include "input_events.h"

#define MAX_DEVICE		10
#define ARRAY_END		(-11)
#define BUF_SIZE		1024
#define MAX_FILENAME 128

#define INPUT_ID_TOUCH		0
#define INPUT_ID_KEY		1
#define STR_TOUCH		"TOUCH"
#define STR_KEY			"KEY"
#define INPUT_ID_STR_KEY	"ID_INPUT_KEY=1"
#define INPUT_ID_STR_TOUCH	"ID_INPUT_TOUCHSCREEN=1"
#define INPUT_ID_STR_KEYBOARD	"ID_INPUT_KEYBOARD=1"
#define INPUT_ID_STR_TABLET 	"ID_INPUT_TABLET=1"

typedef struct _input_dev
{
	int fd;
	char fileName[MAX_FILENAME];
} input_dev;

static input_dev g_key_dev[MAX_DEVICE];
static input_dev g_touch_dev[MAX_DEVICE];

static Ecore_Fd_Handler *key_handlers[MAX_DEVICE];
static Ecore_Fd_Handler *touch_handlers[MAX_DEVICE];

enum status_t { s_stopped, s_running };
static enum status_t status = s_stopped;

static const char *input_key_devices[] = {
	/* target: Emulator, kernel: 3.4, all buttons */
	"Maru Virtio Hwkey",
	/* target: M0, kernel: 3.0, buttons: volume +/-, home, power */
	"gpio-keys",
	/* target: M0, kernel: 3.0, buttons: menu, back */
	"melfas-touchkey",
	/* target: M0, kernel: 3.10, buttons: menu, back */
	"MELFAS MCS Touchkey",
	/* target: M0, kernel: 3.10, buttons: volume +/-, home, power */
	"gpio-keys.5",
	NULL
};

static const char *input_touch_devices[] = {
	/* target: Emulator, kernel: 3.0 */
	"Maru Virtio Touchscreen",
	/* target: M0, kernel: 3.0 */
	"sec_touchscreen",
	/* target: M0, kernel: 3.10 */
	"MELPAS MMS114 Touchscreen",
	NULL
};

static int check_input(char *inputname, int input_id)
{
	int ret = -1;
	FILE *cmd_fp = NULL;
	char buffer[BUF_SIZE];
	char command[MAX_FILENAME];
	char **name_arr;
	size_t bytes_count;

	sprintf(command, "/sys/class/input/%s/device/name", inputname);
	// run command
	cmd_fp = fopen(command, "r");
	if (cmd_fp == NULL)
		goto exit;

	buffer[0] = '\0';
	bytes_count = fread(buffer, 1, BUF_SIZE, cmd_fp);
	if (bytes_count <= 1) {
		LOGE("Failed to read input_id\n");
		goto exit;
	} else {
		buffer[bytes_count - 1] = '\0';
	}

	if (input_id == INPUT_ID_KEY)
		name_arr = input_key_devices;
	else if (input_id == INPUT_ID_TOUCH)
		name_arr = input_touch_devices;
	else
		goto exit;

	while (*name_arr != NULL) {
		if (strcmp(buffer, *name_arr) == 0) {
			ret = 0;
			goto exit;
		}
		name_arr++;
	}

exit:
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

	if (dp == NULL)
		goto exit;

	while ((d = readdir(dp)) != NULL) {
		if (!strncmp(d->d_name, "event", 5)) {
			// start with "event"
			// event file
			if (!check_input(d->d_name, input_id)) {
				sprintf(dev[count].fileName,
					"/dev/input/%s", d->d_name);
				dev[count].fd = open(dev[count].fileName,
						     O_RDWR | O_NONBLOCK);
				count++;
			}
		}
	}

	closedir(dp);

exit:
	dev[count].fd = ARRAY_END;	// end of input_dev array
}

#define MAX_EVENTS_NUM 10
static int deviceEventHandler(input_dev *dev, int input_type)
{
	int ret = 0;
	ssize_t size = 0;
	int count = 0;
	struct input_event in_ev[MAX_EVENTS_NUM];
	struct msg_data_t *log;

	if (input_type == INPUT_ID_TOUCH || input_type == INPUT_ID_KEY) {
		do {
			size = read(dev->fd, &in_ev[count], sizeof(*in_ev));
			if (size > 0)
				count++;
		} while (count < MAX_EVENTS_NUM && size > 0);

		if (count) {
			LOGI("read %d %s events\n",
			     count,
			     input_type == INPUT_ID_KEY ? STR_KEY : STR_TOUCH);
			log = gen_message_event(in_ev, count, input_type);
			printBuf((char *)log, MSG_DATA_HDR_LEN + log->len);
			if (write_to_buf(log) != 0)
				LOGE("write to buf fail\n");
			free_msg_data(log);
		}
	} else {
		LOGW("unknown input_type\n");
		ret = 1;	// it is not error
	}
	return ret;
}

static Eina_Bool touch_event_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	input_dev *touch_dev = (input_dev *)data;

	if (deviceEventHandler(touch_dev, INPUT_ID_TOUCH) < 0) {
		LOGE("Internal DA framework error, "
		     "Please re-run the profiling (touch dev)\n");
		/* TODO: ??? */
	}

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool key_event_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	input_dev *key_dev = (input_dev *)data;

	if (deviceEventHandler(key_dev, INPUT_ID_KEY) < 0) {
		LOGE("Internal DA framework error, "
		     "Please re-run the profiling (key dev)\n");
		/* TODO: ??? */
	}

	return ECORE_CALLBACK_RENEW;
}

void add_input_events(void)
{
	int i;

	if (status == s_running)
		return;

	for (i = 0; g_key_dev[i].fd != ARRAY_END; i++) {
		if (g_key_dev[i].fd > 0) {
			key_handlers[i] =
				ecore_main_fd_handler_add(g_key_dev[i].fd,
							  ECORE_FD_READ,
							  key_event_cb,
							  &g_key_dev[i],
							  NULL, NULL);
			if (!key_handlers[i])
				LOGE("keyboard device file handler add error\n");
		}
	}

	for (i = 0; g_touch_dev[i].fd != ARRAY_END; i++) {
		if (g_touch_dev[i].fd > 0) {
			touch_handlers[i] =
				ecore_main_fd_handler_add(g_touch_dev[i].fd,
							  ECORE_FD_READ,
							  touch_event_cb,
							  &g_touch_dev[i],
							  NULL, NULL);
			if (!touch_handlers[i])
				LOGE("touch device file handler add error\n");
		}
	}

	status = s_running;
}

void del_input_events(void)
{
	int i;

	if (status == s_stopped)
		return;

	for (i = 0; g_key_dev[i].fd != ARRAY_END; i++)
		if (g_key_dev[i].fd > 0)
			ecore_main_fd_handler_del(key_handlers[i]);

	for (i = 0; g_touch_dev[i].fd != ARRAY_END; i++)
		if (g_touch_dev[i].fd > 0)
			ecore_main_fd_handler_del(touch_handlers[i]);

	status = s_stopped;
}

static void _device_write(input_dev dev[], struct input_event *in_ev)
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

void write_input_event(int id, struct input_event *ev)
{
	switch (id) {
	case INPUT_ID_TOUCH:
		_device_write(g_touch_dev, ev);
		break;
	case INPUT_ID_KEY:
		_device_write(g_key_dev, ev);
		break;
	default:
		LOGE("unknown input id (%d)\n", id);
	}
}

int init_input_events(void)
{
	_get_fds(g_key_dev, INPUT_ID_KEY);
	if (g_key_dev[0].fd == ARRAY_END) {
		LOGE("No key devices found.\n");
		return -1;
	}
	_get_fds(g_touch_dev, INPUT_ID_TOUCH);
	if (g_touch_dev[0].fd == ARRAY_END) {
		LOGE("No touch devices found.\n");
		return -1;
	}

	return 0;
}
