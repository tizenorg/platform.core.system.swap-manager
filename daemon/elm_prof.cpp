/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) Samsung Electronics, 2014
 *
 * 2014         Vyacheslav Cherkashin <v.cherkashin@samsung.com>
 *
 */


#include <list>
#include <string>
#include <sstream>

#include <app.h>

#include "debug.h"
#include "elm_prof.h"
#include "da_protocol.h"


static const char path_cmd[] = "/sys/kernel/debug/swap/elm_prof/cmd";
static const char path_enabled[] = "/sys/kernel/debug/swap/elm_prof/enabled";
struct app_info {
	uint64_t main;
	uint64_t plt_aem;
	std::string path;
};

typedef std::list <app_info> app_info_list;

static app_info_list g_app_list;
static app_info_list g_app_list_inst;


static int get_create_cb_offset(void)
{
	return offsetof(app_event_callback_s, create);
}

static int get_service_cb_offset(void)
{
	return offsetof(app_event_callback_s, app_control);
}

static std::string int2str(int val)
{
	std::stringstream ss;
	ss << val;
	return ss.str();
}

static std::string int2hex(int val)
{
	std::stringstream ss;
	ss << std::hex << val;
	return ss.str();
}


static ssize_t write_to_file(const char *filename, const void *buf, size_t len)
{
	int fd;
	ssize_t ss;

	fd = open(filename, O_WRONLY);
	if (fd == -1)
		return -errno;

	ss = write(fd, buf, len);
	if (ss == -1)
		ss = -errno;

	close(fd);

	return ss;
}

static ssize_t write_to_file(const std::string &filename, const std::string &cmd)
{
	return write_to_file(filename.c_str(), cmd.c_str(), cmd.size());
}


static int elm_prof_add_app(const struct app_info &info)
{
	ssize_t ret;
	std::string cmd;

	cmd = "a 0x" + int2hex(info.plt_aem) +
	      ":0x" + int2hex(info.main) + ":" + info.path;

	ret = write_to_file(path_cmd, cmd);

	return ret > 0 ? 0 : ret;
}

static int elm_prof_rm_app(const struct app_info &info)
{
	ssize_t ret;
	std::string cmd("r " + info.path);;

	ret = write_to_file(path_cmd, cmd);

	return ret > 0 ? 0 : ret;
}


static void unreg_list(const app_info_list &app_list)
{
	for (app_info_list::const_iterator it = app_list.begin();
	     it != app_list.end(); ++it) {
		elm_prof_rm_app(*it);
	}
}

static int reg_list(const app_info_list &app_list)
{
	int ret;

	for (app_info_list::const_iterator it = app_list.begin();
	     it != app_list.end(); ++it) {
		ret = elm_prof_add_app(*it);
		if (ret) {
			unreg_list(app_list);
			return ret;
		}
	}

	return 0;
}


static int parsing_app_info(struct msg_buf_t *msg_buf, struct app_info &info)
{
	char *path;

	if (!parse_string(msg_buf, &path)) {
		LOGE("app path parsing error\n");
		return -EINVAL;
	}

	info.path = path;
	free(path);

	if (!parse_int64(msg_buf, &info.main)) {
		LOGE("main address parsing error\n");
		return -EINVAL;
	}

	if (!parse_int64(msg_buf, &info.plt_aem)) {
		LOGE("plt app_efl_main address parsing error\n");
		return -EINVAL;
	}

	return 0;
}

extern "C" int process_msg_elm_prof(struct msg_buf_t *msg_buf)
{
	uint32_t cnt;
	app_info_list app_list;

	if (!parse_int32(msg_buf, &cnt)) {
		LOGE("count parsing error\n");
		return -EINVAL;
	}

	for (int i = 0; i < cnt; ++i) {
		int ret;
		struct app_info info;

		ret = parsing_app_info(msg_buf, info);
		if (ret) {
			return ret;
		}

		app_list.push_back(info);
	}

	unreg_list(g_app_list_inst);
	g_app_list = app_list;

	return 0;
}


static int sent_comm_info()
{
	int ret;
	pid_t tgid;
	std::string cmd;

	tgid = find_pid_from_path(DEBUG_LAUNCH_PRELOAD_PATH);
	if (tgid == 0) {
		LOGE("not found PID for %s\n", DEBUG_LAUNCH_PRELOAD_PATH);
		return -ESRCH;
	}

	cmd = "s offset_create " + int2str(get_create_cb_offset());
	ret = write_to_file(path_cmd, cmd);
	if (ret < 0)
		return ret;

	cmd = "s offset_service " + int2str(get_service_cb_offset());
	ret = write_to_file(path_cmd, cmd);
	if (ret < 0)
		return ret;

	cmd = "s debug_lpad_tgid " + int2str(tgid);
	ret = write_to_file(path_cmd, cmd);

	return ret > 0 ? 0 : ret;
}

static int set_enable()
{
	int ret;

	ret = sent_comm_info();
	if (ret)
		return ret;

	ret = write_to_file(path_enabled, std::string("1"));

	return ret > 0 ? 0 : ret;
}

static int set_disable()
{
	int ret;

	ret = write_to_file(path_enabled, std::string("0"));

	return ret > 0 ? 0 : ret;
}

static void set_off()
{
	set_disable();

	unreg_list(g_app_list_inst);
	g_app_list_inst.clear();
}

static int set_on()
{
	int ret;

	set_off();

	ret = reg_list(g_app_list);
	if (ret)
		return ret;

	g_app_list_inst = g_app_list;

	ret = set_enable();
	if (ret)
		set_off();

	return 0;
}

extern "C" int elm_prof_set(enum elm_prof_state state)
{
	switch (state) {
	case EPS_ON:
		return set_on();
	case EPS_OFF:
		set_off();
		return 0;
	}

	return -EINVAL;
}
