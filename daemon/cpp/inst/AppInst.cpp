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


#include "AppInst.h"
#include "AppInstTizen.h"
#include "AppInstRunning.h"
#include "AppInstCommon.h"
#include "AppInstWeb.h"
#include "swap_debug.h"


AppInst *AppInst::create(AppType type, const AppInstInfo &info)
{
    AppInst *app(0);

    switch (type) {
    case AT_TIZEN:
        app = new AppInstTizen();
        break;
    case AT_RUNNING:
        app = new AppInstRunning();
        break;
    case AT_COMMON:
        app = new AppInstCommon();
        break;
    case AT_WEB:
        app = new AppInstWeb();
        break;
    default:
        LOGE("'app type' incorrect, type=%d\n", type);
        return 0;
    }

    if (app) {
        int ret = app->init(info);
        if (ret) {
            LOGE("[%s] initialization error, ret=%d\n", type2str(type), ret);
            delete app;
            app = 0;
        }
    }

    return app;
}

void AppInst::destroy(AppInst *app)
{
    app->uninit();
    delete app;
}

int AppInst::init(const AppInstInfo &info)
{
    _info = new AppInstInfo(info);
    if (_info == 0)
        return -ENOMEM;

    return doInit();
}

void AppInst::uninit()
{
    doUninit();
    delete _info;
    _info = 0;
}

