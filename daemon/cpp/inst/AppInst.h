/*
 *  DA manager
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *      Vyacheslav Cherkashin <v.cherkashin@samsung.com>
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
 * - Samsung RnD Institute Russia
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
