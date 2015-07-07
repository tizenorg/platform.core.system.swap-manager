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


#ifndef APPINSTCOMMON_H
#define APPINSTCOMMON_H

#include "AppInst.h"
#include "common.h"
#include "swap_debug.h"


class AppInstCommon : public AppInst
{
    int doInit()
    {
        if (info().id().size()) {
            LOGE("[%s] error 'app ID' is not empty '%s'\n",
                 type2str(AT_COMMON), info().id().c_str());
            return -EINVAL;
        }

        return 0;
    }
};


#endif // APPINSTCOMMON_H
