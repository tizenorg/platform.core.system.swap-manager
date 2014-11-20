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
#include <mutex>
#include <string>
#include <sstream>

#include <app.h>

#include "debug.h"
#include "elm_prof.h"
#include "da_protocol.h"


static const char path_cmd[] = "/sys/kernel/debug/swap/elm_prof/cmd";
static const char path_enabled[] = "/sys/kernel/debug/swap/elm_prof/enabled";


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




struct app_info {
	app_info(const char *path, uint64_t main, uint64_t plt_aem)
	{
		this->path = path;
		this->main = main;
		this->plt_aem = plt_aem;
	}

	uint64_t main;
	uint64_t plt_aem;
	std::string path;
};

typedef std::list <app_info> app_info_list;
typedef std::lock_guard <std::mutex> lock_guard_mutex;


static int elm_prof_add_app(const struct app_info &info)
{
	ssize_t ret;
	std::string cmd;

	cmd = "a 0x" + int2hex(info.plt_aem) +
	      ":0x" + int2hex(info.main) + ":" + info.path;

	ret = write_to_file(path_cmd, cmd);

	return ret > 0 ? 0 : ret;
}

static int elm_prof_rm_app(const std::string &path)
{
	ssize_t ret;
	std::string cmd("r " + path);;

	ret = write_to_file(path_cmd, cmd);

	return ret > 0 ? 0 : ret;
}

static int elm_prof_rm_app(const struct app_info &info)
{
	return elm_prof_rm_app(info.path);
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


static app_info_list::iterator app_info_list_find(app_info_list &app_list,
						  const std::string &path)
{
	app_info_list::iterator it_end = app_list.end();

	for (app_info_list::iterator it = app_list.begin();
	     it != it_end; ++it) {
		if (it->path == path)
			return it;
	}

	return it_end;
}

static int do_elm_prof_add(const app_info &info, app_info_list &app_list)
{
	int ret;

	if (app_list.end() != app_info_list_find(app_list, info.path))
		return -EINVAL;

	ret = elm_prof_add_app(info);
	if (ret)
		return ret;

	sent_comm_info();
	ret = set_enable();
	if (ret) {
		elm_prof_rm_app(info);
		return ret;
	}

	app_list.push_back(info);
	return ret;
}

static void do_elm_prof_rm(const std::string &path, app_info_list &app_list)
{
	app_info_list::iterator it;

	it = app_info_list_find(app_list, path);
	if (it != app_list.end()) {
		elm_prof_rm_app(path);
		app_list.erase(it);
	}

	if (app_list.empty())
		set_disable();
}

static void do_elm_prof_rm_all(app_info_list &app_list)
{
	app_info_list::iterator it_end = app_list.end();

	for (app_info_list::iterator it = app_list.begin(),
	     it_end = app_list.end(); it != it_end; ++it) {
		elm_prof_rm_app(it->path);
	}

	app_list.clear();
	set_disable();
}


static std::mutex g_lock;
static app_info_list g_app_list;

extern "C" int elm_prof_add(const char *path, unsigned long main_addr,
			    unsigned long aem_addr)
{
	int ret;
	app_info info(path, main_addr, aem_addr);
	lock_guard_mutex lock(g_lock);

	return do_elm_prof_add(info, g_app_list);
}

extern "C" void elm_prof_rm(const char *path)
{
	lock_guard_mutex lock(g_lock);

	do_elm_prof_rm(path, g_app_list);
}

extern "C" void elm_prof_rm_all(void)
{
	lock_guard_mutex lock(g_lock);

	do_elm_prof_rm_all(g_app_list);
}
