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


#ifndef _COMMON_H
#define _COMMON_H


#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <sstream>


static inline std::string int2str(int val)
{
        std::stringstream ss;
        ss << val;
        return ss.str();
}

static inline std::string addr2hex(unsigned long val)
{
    std::stringstream ss;
    ss << std::hex << val;
    return "0x" + ss.str();
}

static inline bool str2pid(const char *str, pid_t &pid)
{
    char c;
    std::stringstream ss(str);
    ss >> pid;
    if (ss.fail() || ss.get(c)) {
        // not an integer
        return false;
    }
    return true;
}

static inline bool buf2uint64(const char *buf, size_t len, uint64_t &out)
{
    if (len != 8)
        return false;

    out = 0;
    for (int i = len - 1; i >= 0; --i) {
        out <<= 8;
        out += (uint8_t)buf[i];
    }

    return true;
}

static inline ssize_t write_to_file(const char *filename,
                                    const void *buf, size_t len)
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

static inline ssize_t write_to_file(const std::string &filename,
                                    const std::string &cmd)
{
    return write_to_file(filename.c_str(), cmd.c_str(), cmd.size());
}


#endif /* _COMMON_H */
