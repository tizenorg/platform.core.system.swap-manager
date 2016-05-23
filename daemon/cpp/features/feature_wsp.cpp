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
                    SWAP_LOGE("cannot open ELF file '%s'\n", ewebkit_path);
                    return ret;
                }

                std::string str = rmPlt(name);
                const char *s = str.c_str();
                uint32_t a = 0;

                ret = elf.getAddrPlt(&s, &a, 1);
                elf.close();

                if (ret) {
                    SWAP_LOGE("get plt addr for '%s'\n", s);
                    return ret;
                }

                addr = a;
            }

            if (addr == 0) {
                SWAP_LOGE("address for '%s' is not found\n", name.c_str());
                return -EINVAL;
            }

            const std::string cmd(addr2hex(addr) + " " + name);

            ret = write_to_file(path_cmd, cmd);
            if (ret < 0) {
                SWAP_LOGE("write(err=%d) '%s' to %s\n", ret, cmd.c_str(), path_cmd);
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
