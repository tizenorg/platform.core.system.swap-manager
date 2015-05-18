#ifndef _COMMON_H
#define _COMMON_H

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
 * 2015         Vyacheslav Cherkashin <v.cherkashin@samsung.com>
 *
 */


#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <sstream>


static inline std::string addr2hex(unsigned long val)
{
    std::stringstream ss;
    ss << std::hex << val;
    return "0x" + ss.str();
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
