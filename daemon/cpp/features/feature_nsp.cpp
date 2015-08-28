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


#include <string>
#include <errno.h>
#include "feature.h"
#include "feature_manager.h"

#include "common.h"
#include "utils.h"
#include "swap_debug.h"
#include "nsp_data.h"               /* auto generate */
#include "inst/AppInstTizen.h"
#include "inst/AppInstCont.h"
#include "elf/FileElf.h"


static const char path_enabled[] = "/sys/kernel/debug/swap/nsp/enabled";
static const char path_cmd[] = "/sys/kernel/debug/swap/nsp/cmd";


static int addApp(const AppInstTizen &app)
{
    std::string cmd = "a "
                    + addr2hex(app.mainOffset()) + ":"
                    + app.info().path();

    int ret = write_to_file(path_cmd, cmd);
    if (ret < 0) {
        LOGE("write to file '%s', cmd='%s'\n", path_cmd, cmd.c_str());
        return ret;
    }

    return 0;
}

static int rmAppAll()
{
    int ret = write_to_file(path_cmd, "c");

    return ret > 0 ? 0 : ret;
}

static void funcInst(const AppInstTizen &app, int &ret)
{
    if (ret)
        return;

    ret = addApp(app);
}

static int instApps()
{
    int ret = 0;

    AppInstCont::instance().for_each(funcInst, ret);
    if (ret) {
        int err = rmAppAll();
        if (err)
            LOGE("cannot all apps remove, err=%d\n", err);
    }

    return ret;
}

static int nspEnable()
{
    int ret = write_to_file(path_enabled, "1");
    if (ret < 0) {
        LOGE("cannot NSP enable\n");
        return ret;
    }

    return 0;
}

static int nspDisable()
{
    int ret = write_to_file(path_enabled, "0");
    if (ret < 0) {
        LOGE("cannot NSP enable\n");
        return ret;
    }

    return 0;
}

static uint32_t getAddrPlt(const char *path, const char *name)
{
    FileElf elf;

    if (elf.open(path)) {
        LOGE("cannot open ELF file '%s'\n", path);
        return 0;
    }

    uint32_t addr = 0;
    int ret = elf.getAddrPlt(&name, &addr, 1);
    elf.close();

    return ret ? 0 : addr;
}

static int initLibAppCore()
{
    uint32_t appcoreInitAddr = ADDR_APPCORE_INIT_PLT ?
                                ADDR_APPCORE_INIT_PLT :
                                getAddrPlt(PATH_LIBAPPCORE_EFL, "appcore_init");

    if (appcoreInitAddr == 0) {
        LOGE("not found 'appcore_init@plt' addr in '%s'\n", PATH_LIBAPPCORE_EFL);
        return -EINVAL;
    }

    uint32_t elmRunAddr = ADDR_ELM_RUN_PLT ?
                            ADDR_ELM_RUN_PLT :
                            getAddrPlt(PATH_LIBAPPCORE_EFL, "elm_run");

    if (elmRunAddr == 0) {
        LOGE("not found 'elm_run@plt' addr in '%s'\n", PATH_LIBAPPCORE_EFL);
        return -EINVAL;
    }


    /* cmd: "l appcore_efl_main:__do_app:appcore_init@plt:elm_run@plt:lib_path" */
    std::string cmd = "l "
                    + addr2hex(ADDR_APPCORE_EFL_MAIN) + ":"
                    + addr2hex(ADDR_DO_APP) + ":"
                    + addr2hex(appcoreInitAddr) + ":"
                    + addr2hex(elmRunAddr) + ":"
                    + PATH_LIBAPPCORE_EFL;

    int ret = write_to_file(path_cmd, cmd);
    if (ret < 0) {
        LOGE("write to file '%s', cmd='%s'\n", path_cmd, cmd.c_str());
        return ret;
    }

    return 0;
}

static int initLpad()
{
    uint32_t dlopenAddr = ADDR_DLOPEN_PLT_LPAD ?
                            ADDR_DLOPEN_PLT_LPAD :
                            getAddrPlt(PATH_LAUNCHPAD, "dlopen");

    if (dlopenAddr == 0) {
        LOGE("not found 'dlopen@plt' addr in '%s'\n", PATH_LAUNCHPAD);
        return -EINVAL;
    }

    uint32_t dlsymAddr = ADDR_DLSYM_PLT_LPAD ?
                            ADDR_DLSYM_PLT_LPAD :
                            getAddrPlt(PATH_LAUNCHPAD, "dlsym");

    if (dlopenAddr == 0) {
        LOGE("not found 'dlsym@plt' addr in '%s'\n", PATH_LAUNCHPAD);
        return -EINVAL;
    }

    /* cmd: "l dlopen@plt:dlsym@plt:lpad_path" */
    std::string cmd = "b "
                    + addr2hex(dlopenAddr) + ":"
                    + addr2hex(dlsymAddr) + ":"
                    + PATH_LAUNCHPAD;

    int ret = write_to_file(path_cmd, cmd);
    if (ret < 0) {
        LOGE("write to file '%s', cmd='%s'\n", path_cmd, cmd.c_str());
        return ret;
    }

    return 0;
}

class FeatureNSP : public Feature
{
    int doInit()
    {
        int ret;

        ret = initLibAppCore();
        if (ret)
            return ret;

        ret = initLpad();

        return ret;
    }

    int doEnable()
    {
        int ret = instApps();
        if (ret)
            LOGE("cannot all apps install\n");

        ret = nspEnable();

        return ret;
    }

    int doDisable()
    {
        nspDisable();

        int ret = rmAppAll();
        if (ret)
            LOGE("cannot all apps remove, ret=%d\n", ret);

        return ret;
    }

    std::string _data;
};


REGISTER_FEATURE(FeatureNSP, val2bit(FL_APP_STARTUP), "NSP");
