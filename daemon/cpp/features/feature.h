#ifndef FEATURE_H
#define FEATURE_H

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


#include <mutex>
#include <string>


/* calculate max bit from number */
template<unsigned long long val>
struct max_bit
{
    enum { num = max_bit<(val >> 1)>::num + 1 };
};

template<>
struct max_bit<1>
{
    enum { num = 0 };
};

/* TODO: add a check number of true bits */
#define val2bit(val) (size_t)max_bit<(val)>::num


class Feature
{
public:
    Feature();
    virtual ~Feature();

    int init();
    int uninit();

    int enable();
    int disable();

private:
    std::string name();
    int virtual doInit() { return 0; }
    int virtual doUninit() { return 0; }

    int virtual doEnable() { return 0; }
    int virtual doDisable() { return 0; }

    enum Flag {
        F_INIT,
        F_ENABLE,
        F_UNINIT
    };

    enum Flag _flag;
    std::mutex _mutex;
};

#endif // FEATURE_H
