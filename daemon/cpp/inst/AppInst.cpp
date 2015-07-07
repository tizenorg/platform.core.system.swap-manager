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

