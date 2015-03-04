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
#include "../debug.h"   /* TODO: correct path */


/* calculate max bit from number */
template<unsigned long long val>
struct max_bit
{
    enum { num = max_bit<(val >> 1)>::num + 1 };
};

template<>
struct max_bit<1>
{
    enum { num = 0 };
};

/* TODO: add a check number of true bits */
#define val2bit(val) max_bit<(val)>::num



typedef std::lock_guard <std::mutex> lock_guard_mutex;


static void fill_array_feaures(Feature *features[FEATURES_MAX])
{
    memset(features, 0, sizeof(*features) * FEATURES_MAX);

    /* fill features */
}

static void clean_array_feaures(Feature *features[FEATURES_MAX])
{
    for (size_t i = 0; i < FEATURES_MAX; ++i)
    {
        if (features[i])
        {
            delete features[i];
            features[i] = 0;
        }
    }
}

FeatureManager::FeatureManager() :
    _st(Off)
{
    fill_array_feaures(_features);
}

FeatureManager::~FeatureManager()
{
    clean_array_feaures(_features);
}

FeatureManager::ErrCode FeatureManager::init()
{
    for (size_t i = 0; i < FEATURES_MAX; ++i)
    {
        int ret;
        Feature *feature = _features[i];
        if (feature == 0)
            continue;

        ret = feature->init();
        if (ret)
        {
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
    for (size_t i = 0; i < FEATURES_MAX; ++i)
    {
        Feature *feature = _features[i];
        if (feature == 0)
            continue;

        if (_bs_init.test(i))
        {
            feature->uninit();
            _bs_init.set(i, false);
        }
    }

    return Ok;
}

FeatureManager::ErrCode FeatureManager::start()
{
    lock_guard_mutex lock(_mutex);

    if (_st == On)
    {
        LOGE("allredy start\n");
        return Err;
    }

    _st = On;
    ErrCode ret = doSetFeatures(_bs_new);
    if (ret != Ok)
    {
        _st = Off;
        shutdownFeatures();
    }

    return ret;
}

FeatureManager::ErrCode FeatureManager::stop()
{
    lock_guard_mutex lock(_mutex);

    if (_st == Off)
    {
        LOGE("allredy stop");
        return Err;
    }

    _bs_new.reset();
    ErrCode ret = doSetFeatures(_bs_new);
    if (ret != Ok)
        shutdownFeatures();
    _st = Off;

    return ret;
}

FeatureManager::ErrCode FeatureManager::doSetFeatures(const feature_bs &f)
{
    if (_st == Off)
    {
        _bs_new = f;
        return Ok;
    }

    feature_bs change(f ^ _bs_cur);

    for (size_t i = 0; i < FEATURES_MAX; ++i)
    {
        if (change.test(i) == false)
            continue;

        if (_bs_init.test(i) == false)
        {
            LOGE("features num=%d is not init\n", i);
            return Err;
        }

        Feature *feature = _features[i];
        if (feature == 0)
        {
            LOGE("no features num=%d\n", i);
            return Err;
        }

        if (_bs_cur.test(i) == false)
        {
            int ret = feature->enable();
            if (ret)
            {
                LOGE("features num=%d is not enable\n", i);
                return Err;
            }
        }
        else
        {
            int ret = feature->disable();
            if (ret)
            {
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

    for (size_t i = 0; i < FEATURES_MAX; ++i)
    {
        if (_bs_init.test(i) == false)
            continue;

        Feature *feature = _features[i];
        if (feature == 0)
        {
            LOGE("no features num=%d\n", i);
            continue;
        }

        if (_bs_cur.test(i))
        {
            int ret = feature->disable();
            if (ret)
            {
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
