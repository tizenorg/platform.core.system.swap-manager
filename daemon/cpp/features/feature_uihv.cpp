/*
 *  DA manager
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd. All rights reserved.
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


#include "feature.h"
#include "feature_manager.h"
#include "smack.h"
#include "common.h"
#include "inst/AppInstTizen.h"
#include "inst/AppInstCont.h"


static const char path_enabled[] = "/sys/kernel/debug/swap/preload/ui_viewer_enabled";
static const char app_info_path[] = "/sys/kernel/debug/swap/preload/ui_viewer_app_info";

static int setApp(const AppInstTizen &app)
{
    static const char object[] = "swap";
    static const char access_type[] = "r";
    const char *app_id = app.info().id().c_str();

    int ret = apply_smack_rules(app_id, object, access_type);
    if (ret) {
        LOGE("smack rules, ret=%d subject='%s' object='%s' access_type='%s'\n",
             ret, app_id, object, access_type);
        return ret;
    }

    std::string cmd = addr2hex(app.mainOffset()) + ":" + app.info().path();

    ret = write_to_file(app_info_path, cmd);

    if (ret < 0) {
        LOGE("write to file '%s', cmd='%s'\n", app_info_path, cmd.c_str());
        return ret;
    }

    return 0;
}

static void funcInst(const AppInstTizen &app, int &ret)
{
    if (ret)
        return;

    ret = setApp(app);
}

static int instApps()
{
    int ret = 0;

    AppInstCont::instance().for_each(funcInst, ret);

    return ret;
}

static int uihvEnable()
{
    int ret = write_to_file(path_enabled, "1");
    if (ret < 0) {
        LOGE("cannot UIHV enable\n");
        return ret;
    }

    return 0;
}

static int uihvDisable()
{
    int ret = write_to_file(path_enabled, "0");
    if (ret < 0) {
        LOGE("cannot UIHV disable\n");
        return ret;
    }

    return 0;
}


class FeatureUIHV : public Feature
{
    int doEnable()
    {
        int ret = instApps();
        if (ret)
            LOGE("cannot all apps install\n");

        ret = uihvEnable();

        return ret;
    }

    int doDisable()
    {
        return uihvDisable();
    }

    std::string _data;
};


REGISTER_FEATURE(FeatureUIHV, val2bit(FL_UI_VIEWER_PROFILING), "UIHV");
