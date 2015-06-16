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
#include <appcore/appcore-common.h>
#include <pkgmgr-info.h>
#include "feature.h"
#include "feature_manager.h"

#include "common.h"
#include "utils.h"
#include "debug.h"
#include "nsp_data.h"               /* auto generate */
#include "inst/AppInstTizen.h"
#include "inst/AppInstCont.h"
#include "elf/FileElf.h"


static const char path_enabled[] = "/sys/kernel/debug/swap/nsp/enabled";
static const char path_cmd[] = "/sys/kernel/debug/swap/nsp/cmd";


enum {
    CB_OFFSET_CRAEATE = offsetof(struct appcore_ops, create),
    CB_OFFSET_RESET = offsetof(struct appcore_ops, reset)
};


static int addApp(const AppInstTizen &app)
{
    int ret;
    pkgmgrinfo_appinfo_h handle;
    bool isProcessPool = 0;
    const char *appId;

    appId = app.info().id().c_str();

    ret = pkgmgrinfo_appinfo_get_appinfo(appId, &handle);
    if (ret < 0) {
        LOGE("can't get pkginfo for '%s'\n", appId);
        return ret;
    }
    ret = pkgmgrinfo_appinfo_is_process_pool(handle, &isProcessPool);
    pkgmgrinfo_appinfo_destroy_appinfo(handle);
    if (ret < 0) {
        LOGE("can't get process pool\n");
        return ret;
    }

    std::string cmd("a " + int2str(isProcessPool) + ":" + app.info().path());

    ret = write_to_file(path_cmd, cmd);
    if (ret < 0) {
        LOGE("write to file '%s', cmd=%s\n", path_cmd, cmd.c_str());
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

class FeatureNSP : public Feature
{
    int doInit()
    {
        int ret;
        std::string cmd;

        cmd = "s offset_create " + int2str(CB_OFFSET_CRAEATE);
        ret = write_to_file(path_cmd, cmd);
        if (ret < 0) {
            LOGE("write to file '%s', cmd=%s\n", path_cmd, cmd.c_str());
            return ret;
        }

        cmd = "s offset_reset " + int2str(CB_OFFSET_RESET);
        ret = write_to_file(path_cmd, cmd);
        if (ret < 0) {
            LOGE("write to file '%s', cmd=%s\n", path_cmd, cmd.c_str());
            return ret;
        }

        uint32_t dlopenAddrPool = ADDR_DLOPEN_PLT_LPAD_POOL;
        if (dlopenAddrPool == 0)
            dlopenAddrPool = getAddrPlt(PATH_LAUNCHPAD_POOL, "dlopen");

        if (dlopenAddrPool == 0) {
            LOGE("not found 'dlopen@plt' addr in '%s'\n", PATH_LAUNCHPAD_POOL);
            return -EINVAL;
        }

        uint32_t dlsymAddrPool = ADDR_DLSYM_PLT_LPAD_POOL;
        if (dlsymAddrPool == 0)
            dlopenAddrPool = getAddrPlt(PATH_LAUNCHPAD_POOL, "dlsym");

        if (dlopenAddrPool == 0) {
            LOGE("not found 'dlsym@plt' addr in '%s'\n", PATH_LAUNCHPAD_POOL);
            return -EINVAL;
        }

        cmd = "b 1:" + addr2hex(dlopenAddrPool) + ":"
            + addr2hex(dlsymAddrPool) + ":" + PATH_LAUNCHPAD_POOL;
        ret = write_to_file(path_cmd, cmd);
        if (ret < 0) {
            LOGE("write to file '%s', cmd=%s\n", path_cmd, cmd.c_str());
            return ret;
        }

        uint32_t dlopenAddrDaemon = ADDR_DLOPEN_PLT_LPAD_DAEMON;
        if (dlopenAddrDaemon == 0)
            dlopenAddrDaemon = getAddrPlt(PATH_LAUNCHPAD_DAEMON, "dlopen");

        if (dlopenAddrDaemon == 0) {
            LOGE("not found 'dlopen@plt' addr in '%s'\n", PATH_LAUNCHPAD_DAEMON);
            return -EINVAL;
        }

        uint32_t dlsymAddrDaemon = ADDR_DLSYM_PLT_LPAD_DAEMON;
        if (dlsymAddrDaemon == 0)
            dlopenAddrDaemon = getAddrPlt(PATH_LAUNCHPAD_DAEMON, "dlsym");

        if (dlopenAddrDaemon == 0) {
            LOGE("not found 'dlsym@plt' addr in '%s'\n", PATH_LAUNCHPAD_DAEMON);
            return -EINVAL;
        }

        cmd = "b 0:" + addr2hex(dlopenAddrDaemon) + ":"
            + addr2hex(dlsymAddrDaemon) + ":" + PATH_LAUNCHPAD_DAEMON;
        ret = write_to_file(path_cmd, cmd);
        if (ret < 0) {
            LOGE("write to file '%s', cmd=%s\n", path_cmd, cmd.c_str());
            return ret;
        }

        cmd = "l " + addr2hex(ADDR_APPCORE_EFL_MAIN) + ":" + PATH_LIBAPPCORE_EFL;
        ret = write_to_file(path_cmd, cmd);
        if (ret < 0) {
            LOGE("write to file '%s', cmd=%s\n", path_cmd, cmd.c_str());
            return ret;
        }

        return 0;
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
