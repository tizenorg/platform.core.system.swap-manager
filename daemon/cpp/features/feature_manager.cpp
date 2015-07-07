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
 * Copyright (C) Samsung Electronics, 2014
 *
 * 2015         Vyacheslav Cherkashin <v.cherkashin@samsung.com>
 *
 */

#include "feature_manager.h"
#include "feature.h"
#include "swap_debug.h"


typedef std::lock_guard <std::mutex> lock_guard_mutex;


FeatureManager::FeatureManager() :
    _st(Off)
{
}

FeatureManager::~FeatureManager()
{
}

Feature *FeatureManager::getFeature(size_t num)
{
    return FeatureRegister::feature(num);
}

FeatureManager::ErrCode FeatureManager::init()
{
    for (size_t i = 0; i < FEATURES_MAX; ++i) {
        int ret;
        Feature *feature = getFeature(i);
        if (feature == 0)
            continue;

        ret = feature->init();
        if (ret) {
            LOGE("Error init feature num=%d\n", i);
            uninit();
            return Err;
        }

        _bs_init.set(i);
    }

    return Ok;
}

FeatureManager::ErrCode FeatureManager::uninit()
{
    lock_guard_mutex lock(_mutex);

    shutdownFeatures();
    for (size_t i = 0; i < FEATURES_MAX; ++i) {
        Feature *feature = getFeature(i);
        if (feature == 0)
            continue;

        if (_bs_init.test(i)) {
            feature->uninit();
            _bs_init.set(i, false);
        }
    }

    return Ok;
}

FeatureManager::ErrCode FeatureManager::start()
{
    lock_guard_mutex lock(_mutex);

    if (_st == On) {
        LOGE("allredy start\n");
        return Err;
    }

    _st = On;
    ErrCode ret = doSetFeatures(_bs_new);
    if (ret != Ok) {
        _st = Off;
        shutdownFeatures();
    }

    return ret;
}

FeatureManager::ErrCode FeatureManager::stop()
{
    lock_guard_mutex lock(_mutex);

    if (_st == Off) {
        LOGE("allredy stop");
        return Err;
    }

    const feature_bs bs_save(_bs_new);
    _bs_new.reset();
    ErrCode ret = doSetFeatures(_bs_new);
    if (ret != Ok)
        shutdownFeatures();
    _st = Off;
    _bs_new = bs_save;

    return ret;
}

FeatureManager::ErrCode FeatureManager::doSetFeatures(const feature_bs &f)
{
    if (_st == Off) {
        _bs_new = f;
        return Ok;
    }

    feature_bs change(f ^ _bs_cur);

    for (size_t i = 0; i < FEATURES_MAX; ++i) {
        if (change.test(i) == false)
            continue;

        if (_bs_init.test(i) == false) {
            LOGE("features num=%d is not init\n", i);
            return Err;
        }

        Feature *feature = getFeature(i);
        if (feature == 0) {
            LOGE("no features num=%d\n", i);
            return Err;
        }

        if (_bs_cur.test(i) == false) {
            int ret = feature->enable();
            if (ret) {
                LOGE("features num=%d is not enable\n", i);
                return Err;
            }
        } else {
            int ret = feature->disable();
            if (ret) {
                LOGE("features num=%d is not enable\n", i);
                return Err;
            }
        }

        _bs_cur.set(i, !_bs_cur.test(i));
    }

    return Ok;
}

void FeatureManager::shutdownFeatures()
{
    _bs_new.reset();

    for (size_t i = 0; i < FEATURES_MAX; ++i) {
        if (_bs_init.test(i) == false)
            continue;

        Feature *feature = getFeature(i);
        if (feature == 0) {
            LOGE("no features num=%d\n", i);
            continue;
        }

        if (_bs_cur.test(i)) {
            int ret = feature->disable();
            if (ret) {
                LOGE("features num=%d is not enable\n", i);
                continue;
            }
        }

        _bs_cur.set(i, false);
    }
}

FeatureManager::ErrCode FeatureManager::setFeatures(const feature_bs &f)
{
    LOGI("%s\n", f.to_string().c_str());

    lock_guard_mutex lock(_mutex);

    return doSetFeatures(f);
}








// FeatureRegister

static const char defFeatureName[] = "UNKNOWN";

const size_t FeatureRegister::UNKNOWN_FEATURE_NUM = -1;
Feature *FeatureRegister::_features[FEATURES_MAX];
const char *FeatureRegister::_names[FEATURES_MAX];

FeatureRegister::FeatureRegister(Feature *feature, size_t num, const char *name)
{
    if (num >= FEATURES_MAX) {
        LOGI("invalid feature num=%u\n", num);
        return;
    }

    if (_features[num] != 0) {
        LOGI("feature[%u] is use (%s)u\n", num, featureName(num).c_str());
        return;
    }

    _features[num] = feature;
    _names[num] = name;
}

Feature *FeatureRegister::feature(size_t num)
{
    return num < FEATURES_MAX ? _features[num] : 0;
}

static std::string doFeatureName(const std::string &name, size_t num)
{
    return name + '[' + std::to_string(num) + ']';
}

std::string FeatureRegister::featureName(size_t num)
{
    return feature(num) == 0 ? defFeatureName : doFeatureName(_names[num], num);
}

std::string FeatureRegister::featureName(const Feature *feature)
{
    const size_t num = featureNum(feature);

    return num == UNKNOWN_FEATURE_NUM ?
                defFeatureName :
                doFeatureName(_names[num], num);
}

size_t FeatureRegister::featureNum(const Feature *feature)
{
    for (size_t i = 0; i < FEATURES_MAX; ++i) {
        if (feature == _features[i])
            return i;
    }

    return UNKNOWN_FEATURE_NUM;
}
