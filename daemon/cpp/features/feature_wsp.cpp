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
#include "swap_debug.h"
#include "wsp_data.h"       /* auto generate */
#include "elf/FileElf.h"


static const char path_enabled[] = "/sys/kernel/debug/swap/wsp/enabled";
static const char path_cmd[] = "/sys/kernel/debug/swap/wsp/cmd";
static const char ewebkit_path[] = "/usr/lib/libewebkit2.so";


static std::string rmPlt(const std::string &name)
{
    static const char postfix[] = "@plt";
    static const size_t postfixSize= sizeof(postfix) - 1;

    if (name.size() <= postfixSize)
        return std::string();

    if (name.find(postfix) == std::string::npos)
        return std::string();

    return name.substr(0, name.size() - postfixSize);
}

class FeatureWSP : public Feature
{
    int doInit()
    {
        for (int ret, i = 0; i < wsp_data_cnt; ++i) {
            const std::string name (wsp_data[i].name);
            unsigned long addr = wsp_data[i].addr;

            if (addr == 0) {
                FileElf elf;

                ret = elf.open(ewebkit_path);
                if (ret) {
                    LOGE("cannot open ELF file '%s'\n", ewebkit_path);
                    return ret;
                }

                std::string str = rmPlt(name);
                const char *s = str.c_str();
                uint32_t a;

                ret = elf.getAddrPlt(&s, &a, 1);
                elf.close();

                if (ret) {
                    LOGE("get plt addr for '%s'\n", s);
                    return ret;
                }

                addr = a;
            }

            if (addr == 0) {
                LOGE("address for '%s' is not found\n", name.c_str());
                return -EINVAL;
            }

            const std::string cmd(addr2hex(addr) + " " + name);

            ret = write_to_file(path_cmd, cmd);
            if (ret < 0) {
                LOGE("write(err=%d) '%s' to %s\n", ret, cmd.c_str(), path_cmd);
                return ret;
            }
        }

        return 0;
    }

    int doEnable()
    {
        int ret(write_to_file(path_enabled, "1"));

        return ret > 0 ? 0 : ret;
    }

    int doDisable()
    {
        int ret(write_to_file(path_enabled, "0"));

        return ret > 0 ? 0 : ret;
    }
};


REGISTER_FEATURE(FeatureWSP, val2bit(FL_WEB_STARTUP_PROFILING), "WSP");
