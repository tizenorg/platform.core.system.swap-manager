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