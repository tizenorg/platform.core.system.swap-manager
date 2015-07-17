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


#include <stdint.h>

#include "feature_manager_c.h"
#include "feature_manager.h"
#include "inst/AppInstCont.h"
#include "swap_debug.h"


static std::string u64toString(uint64_t val)
{
    std::string str;

    for (int i = 0; i < sizeof(uint64_t) * 8; ++i) {
        str += (val & 0x8000000000000000LLU) ? '1' : '0';
        val <<= 1;
    }

    return str;
}

static FeatureManager *fm = 0;

extern "C" int fm_init(void)
{
    if (fm) {
        LOGE("allredy init\n");
        return -EINVAL;
    }

    fm = new FeatureManager;
    if (fm->init() != FeatureManager::Ok) {
        LOGE("init FeatureManager\n");
        delete fm;
        fm = 0;
        return -EINVAL;
    }

    return 0;
}

extern "C" void fm_uninit(void)
{
    if (fm == 0) {
        LOGE("FeatureManager in not init\n");
        return;
    }

    fm->uninit();
    delete fm;
    fm = 0;
}

extern "C" int fm_start(void)
{
    if (fm->start() != FeatureManager::Ok) {
        LOGE("start features\n");
        return -EINVAL;
    }

    return 0;
}

extern "C" int fm_stop(void)
{
    if (fm->stop() != FeatureManager::Ok) {
        LOGE("stop features\n");
        return -EINVAL;
    }

    return 0;
}

static uint64_t checkSupportFeatures(uint64_t f, uint64_t fs, size_t offset)
{
    if (f != fs) {
        uint64_t diff = f ^ fs;

        for (int i = 0; diff; diff >>= 1, ++i) {
            if (diff & 1)
                LOGW("feature[%d] is not support\n", i + offset);
        }
    }

    return f & fs;
}

extern "C" int fm_set(uint64_t f0, uint64_t f1)
{
    const uint64_t fSupport0 = checkSupportFeatures(f0, FeatureRegister::f0(), 0);
    const uint64_t fSupport1 = checkSupportFeatures(f1, FeatureRegister::f1(), 64);
    const std::string f(u64toString(fSupport1) + u64toString(fSupport0));

    if (fm->setFeatures(feature_bs(f)) != FeatureManager::Ok) {
        LOGE("set features\n");
        return -EINVAL;
    }

    return 0;
}

static AppType to_apptype(uint32_t val)
{
    switch (val) {
    case APP_TYPE_TIZEN:
        return AT_TIZEN;
    case APP_TYPE_RUNNING:
        return AT_RUNNING;
    case APP_TYPE_COMMON:
        return AT_COMMON;
    case APP_TYPE_WEB:
        return AT_WEB;
    }

    return AT_UNKNOWN;
}


extern "C" int fm_app_add(uint32_t app_type, const char *id, const char *path,
                          const void *data, size_t size)
{
    ByteArray ba(size);
    memcpy(ba.data(), data, size);

    int ret = AppInstCont::instance().add(to_apptype(app_type), id, path, ba);
    if (ret)
        LOGE("add app error, ret=%d\n", ret);

    return ret;
}

extern "C" void fm_app_clear(void)
{
    AppInstCont::instance().clear();
}
