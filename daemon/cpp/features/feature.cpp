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

#include <errno.h>

#include "feature.h"
#include "feature_manager.h"
#include "swap_debug.h"


typedef std::lock_guard <std::mutex> lock_guard_mutex;

Feature::Feature() :
    _flag(F_UNINIT)
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

    switch (_flag) {
    case F_INIT:
    case F_ENABLE:
        SWAP_LOGE("allredy init feature '%s'\n", name().c_str());
        return -EINVAL;

    case F_UNINIT:
        int ret = doInit();
        if (ret) {
            SWAP_LOGE("init feature '%s', err=%d\n", name().c_str(), ret);
            return ret;
        }
        break;
    }

    _flag = F_INIT;
    return 0;
}

int Feature::uninit()
{
    lock_guard_mutex lock(_mutex);

    int ret;

    switch (_flag) {
    case F_ENABLE:
        SWAP_LOGW("feature '%s' disable\n", name().c_str());
        ret = doDisable();
        if (ret)
            return ret;
        /* no break. fall throught to 'case F_INIT' */
    case F_INIT:
        ret = doUninit();
        if (ret) {
            SWAP_LOGE("uninit feature '%s', ret=%d\n", name().c_str(), ret);
            return ret;
        }
        break;

    case F_UNINIT:
        SWAP_LOGE("feature '%s' is not init\n", name().c_str());
        return -EINVAL;
    }

    _flag = F_UNINIT;
    return 0;
}

int Feature::enable()
{
    lock_guard_mutex lock(_mutex);

    switch (_flag) {
    case F_UNINIT:
        SWAP_LOGE("feature '%s' is not init\n", name().c_str());
        return -EINVAL;

    case F_ENABLE:
        SWAP_LOGE("allredy enable feature '%s'\n", name().c_str());
        return -EINVAL;

    case F_INIT:
        int ret = doEnable();
        if (ret) {
            SWAP_LOGE("enable feature '%s', ret=%d\n", name().c_str(), ret);
            return ret;
        }
    }

    _flag = F_ENABLE;
    return 0;
}

int Feature::disable()
{
    lock_guard_mutex lock(_mutex);

    switch (_flag) {
    case F_INIT:
        SWAP_LOGE("already disable feature '%s'\n", name().c_str());
        return -EINVAL;

    case F_UNINIT:
        SWAP_LOGE("feature '%s' is not init\n", name().c_str());
        return -EINVAL;

    case F_ENABLE:
        int ret = doDisable();
        if (ret) {
            SWAP_LOGE("disabe feature '%s', ret=%d\n", name().c_str(), ret);
            return ret;
        }
    }

    _flag = F_INIT;
    return 0;
}
