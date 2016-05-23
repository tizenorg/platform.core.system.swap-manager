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


#include <list>
#include <string>
#include <cstdio>
#include "Anr.h"
#include "swap_debug.h"


static const char EXCLUDE_PATH[] = "/opt/usr/etc/resourced_proc_exclude.ini";

static void rm_endline_symbols(char *buf, size_t len)
{
    char *p, *buf_end;

    buf_end = buf + len;
    for (p = buf; p != buf_end; ++p)
        if (*p == '\n' || *p == '\r')
            *p = '\0';
}


namespace Anr {

int add(const std::string &name)
{
    int ret = 0;

    FILE *f = fopen(EXCLUDE_PATH, "a+");
    if (f == NULL)
        return -errno;

    std::string line(name + "\n");
    if (fwrite(line.c_str(), line.length(), 1, f) != 1)
        ret = -errno;

    fclose(f);
    return ret;
}

int del(const std::string &name)
{
    FILE *f = fopen(EXCLUDE_PATH, "r+");
    if (f == NULL)
        return -errno;

    typedef std::list <std::string> StrList;

    char *line = NULL;
    size_t len = 0;
    ssize_t ret;
    StrList strList;

    // read all names
    while ((ret = getline(&line, &len, f)) != -1) {
        rm_endline_symbols(line, ret);
        strList.push_back(line);
    }
    if (line)
            free(line);

    // clean file
    fseek(f, 0L, SEEK_SET);
    ftruncate(fileno(f), 0);

    // write new names list without 'name'
    bool delFlag = false;
    for (StrList::const_iterator it = strList.begin(),
         itEnd = strList.end(); it != itEnd; ++it) {
        if (delFlag == false && *it == name) {
            delFlag = true;
            continue;
        }

        std::string new_line(*it + '\n');
        if (fwrite(new_line.c_str(), new_line.length(), 1, f) != 1)
            SWAP_LOGE("failed write '%s' to file '%s'\n", new_line.c_str(), EXCLUDE_PATH);
    }

    fclose(f);

    return 0;
}

} // namespace Anr
