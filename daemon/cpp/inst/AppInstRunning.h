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
        const char *pid_st = info().id().c_str();
        if (pid_st[0] == '\0') {
                /* if pid is empty string we should instrument all applications */
                _pid = 0; /* posible it is not good idea set 0 */
        } else if (!str2pid(pid_st, _pid)) {
            SWAP_LOGE("[%s] error convert 'app ID to pid' <pid=%s>\n", type2str(AT_RUNNING), pid_st);
            return -EINVAL;
        }

        return 0;
    }

    pid_t _pid;
};

#endif // APPINSTRUNNING_H
