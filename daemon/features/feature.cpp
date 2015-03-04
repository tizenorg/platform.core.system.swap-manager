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

#include <errno.h>

#include "feature.h"
#include "feature_manager.h"
#include "../debug.h"


typedef std::lock_guard <std::mutex> lock_guard_mutex;

Feature::Feature() :
    _flag(FUnint)
{
}

Feature::~Feature()
{
}

std::string Feature::name()
{
    return featureName(this) ;
}

int Feature::init()
{
    lock_guard_mutex lock(_mutex);

    switch (_flag)
    {
    case FInit:
    case FEnable:
        LOGE("allredy init feature '%s'\n", name().c_str());
        return -EINVAL;

    case FUnint:
        int ret = doInit();
        if (ret)
        {
            LOGE("init feature '%s', err=%d\n", name().c_str(), ret);
            return ret;
        }
        break;
    }

    _flag = FInit;
    return 0;
}

int Feature::uninit()
{
    lock_guard_mutex lock(_mutex);

    int ret;

    switch (_flag)
    {
    case FEnable:
        LOGW("feature '%s' disable\n", name().c_str());
        ret = doDisable();
        if (ret)
            return ret;
    case FInit:
        ret = doUninit();
        if (ret)
        {
            LOGE("uninit feature '%s', ret=%d\n", name().c_str(), ret);
            return ret;
        }
        break;

    case FUnint:
        LOGE("feature '%s' is not init\n", name().c_str());
        return -EINVAL;
    }

    _flag = FUnint;
    return 0;
}

int Feature::enable()
{
    lock_guard_mutex lock(_mutex);

    switch (_flag)
    {
    case FUnint:
        LOGE("feature '%s' is not init\n", name().c_str());
        return -EINVAL;

    case FEnable:
        LOGE("allredy enable feature '%s'\n", name().c_str());
        return -EINVAL;

    case FInit:
        int ret = doEnable();
        if (ret)
        {
            LOGE("enable feature '%s', ret=%d\n", name().c_str(), ret);
            return ret;
        }
    }

    _flag = FEnable;
    return 0;
}

int Feature::disable()
{
    lock_guard_mutex lock(_mutex);

    switch (_flag)
    {
    case FInit:
        LOGE("already disable feature '%s'\n", name().c_str());
        return -EINVAL;

    case FUnint:
        LOGE("feature '%s' is not init\n", name().c_str());
        return -EINVAL;

    case FEnable:
        int ret = doDisable();
        if (ret)
        {
            LOGE("disabe feature '%s', ret=%d\n", name().c_str(), ret);
            return ret;
        }
    }

    _flag = FInit;
    return 0;
}
