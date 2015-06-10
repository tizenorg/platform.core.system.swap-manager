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


#ifndef APPINSTTIZEN_H
#define APPINSTTIZEN_H


#include "AppInst.h"
#include "common.h"
#include "debug.h"


class AppInstTizen : public AppInst
{
public:
    uint64_t mainOffset() const { return _mainOffset; }

private:
    int doInit()
    {
        const ByteArray &data = info().data();

        if (!buf2uint64(data.data(), data.size(), _mainOffset)) {
            LOGE("[%s] error convert 'setup data'\n", type2str(AT_TIZEN));
            return -EINVAL;
        }

        return 0;
    }

    uint64_t _mainOffset;
};


#endif // APPINSTTIZEN_H
