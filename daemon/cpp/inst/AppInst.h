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
 * Copyright (C) Samsung Electronics, 2015
 *
 * 2015         Vyacheslav Cherkashin <v.cherkashin@samsung.com>
 *
 */


#ifndef APPINST_H
#define APPINST_H


#include <string>
#include <vector>


typedef std::vector<char> ByteArray;


#define APP_TYPE_LIST \
    X(UNKNOWN) \
    X(TIZEN) \
    X(RUNNING) \
    X(COMMON) \
    X(WEB)


#define X(type) AT_##type,
enum AppType {
    /* AT_xxx */
    APP_TYPE_LIST
};
#undef X

static inline const char *type2str(AppType type)
{
#define X(type) case AT_##type: return #type;
    switch (type) {
    APP_TYPE_LIST
    }
#undef X
    return "error_type";
}


class AppInstInfo
{
public:
    AppInstInfo(const std::string &id, const std::string &path,
                const ByteArray &data) :
        _id(id),
        _path(path),
        _data(data)
    {}

    const std::string &id() const { return _id; }
    const std::string &path() const { return _path; }
    const ByteArray &data() const { return _data; }

private:
    const std::string _id;
    const std::string _path;
    const ByteArray _data;
};


class AppInst
{
public:
    static AppInst *create(AppType type, const AppInstInfo &info);
    static void destroy(AppInst *app);

    const AppInstInfo &info() const { return *_info; }

protected:
    AppInst() : _info(0) {}
    virtual ~AppInst() {}

private:
    int init(const AppInstInfo &info);
    void uninit();

    int virtual doInit() { return 0; }
    void virtual doUninit() {}

    const AppInstInfo *_info;
};


#endif // APPINST_H
