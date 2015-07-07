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


#ifndef APPINSTRUNNING_H
#define APPINSTRUNNING_H


#include "AppInst.h"
#include "common.h"
#include "swap_debug.h"


class AppInstRunning : public AppInst
{
public:
    pid_t pid() { return _pid; }

private:
    int doInit()
    {
        if (!str2pid(info().id().c_str(), _pid)) {
            LOGE("[%s] error convert 'app ID to pid'\n", type2str(AT_RUNNING));
            return -EINVAL;
        }

        return 0;
    }

    pid_t _pid;
};

#endif // APPINSTRUNNING_H
