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

    uint32_t addr;
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
