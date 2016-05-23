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


#include <stdint.h>

#include "feature_manager_c.h"
#include "feature_manager.h"
#include "inst/AppInstCont.h"
#include "swap_debug.h"


static std::string u64toString(uint64_t val)
{
    std::string str;

    for (unsigned int i = 0; i < sizeof(uint64_t) * 8; ++i) {
        str += (val & 0x8000000000000000LLU) ? '1' : '0';
        val <<= 1;
    }

    return str;
}

static FeatureManager *fm = 0;

extern "C" int fm_init(void)
{
    if (fm) {
        SWAP_LOGE("allredy init\n");
        return -EINVAL;
    }

    fm = new FeatureManager;
    if (fm->init() != FeatureManager::Ok) {
        SWAP_LOGE("init FeatureManager\n");
        delete fm;
        fm = 0;
        return -EINVAL;
    }

    return 0;
}

extern "C" void fm_uninit(void)
{
    if (fm == 0) {
        SWAP_LOGE("FeatureManager in not init\n");
        return;
    }

    fm->uninit();
    delete fm;
    fm = 0;
}

extern "C" int fm_start(void)
{
    if (fm->start() != FeatureManager::Ok) {
        SWAP_LOGE("start features\n");
        return -EINVAL;
    }

    return 0;
}

extern "C" int fm_stop(void)
{
    if (fm->stop() != FeatureManager::Ok) {
        SWAP_LOGE("stop features\n");
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
                SWAP_LOGW("feature[%d] is not support\n", i + offset);
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
        SWAP_LOGE("set features\n");
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
        SWAP_LOGE("add app error, ret=%d\n", ret);

    return ret;
}

extern "C" void fm_app_clear(void)
{
    AppInstCont::instance().clear();
}
