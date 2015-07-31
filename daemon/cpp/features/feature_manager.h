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

#ifndef FEATURE_MANAGER_H
#define FEATURE_MANAGER_H


#include <bitset>
#include <mutex>


enum {
    FEATURES_MAX = 128
};
typedef std::bitset <FEATURES_MAX> feature_bs;


class Feature;

class FeatureManager
{
public:
    enum ErrCode {
        Ok,
        Err
    };

    FeatureManager();
    ~FeatureManager();

    ErrCode init();
    ErrCode uninit();

    ErrCode start();
    ErrCode stop();

    ErrCode setFeatures(const feature_bs &bs);

private:
    static Feature *getFeature(size_t num);
    ErrCode doSetFeatures(const feature_bs &f);
    void shutdownFeatures();

    enum State {
        On,
        Off
    };

    State _st;
    std::mutex _mutex;

    feature_bs _bs_cur;
    feature_bs _bs_new;
    feature_bs _bs_init;
};


class FeatureRegister
{
public:
    FeatureRegister(Feature *feature, size_t num, const char *name);

    static Feature *feature(size_t num);
    static std::string featureName(size_t num);
    static std::string featureName(const Feature *feature);
    static size_t featureNum(const Feature *feature);
    static uint64_t f0() { return _f0; }
    static uint64_t f1() { return _f1; }

    static const size_t UNKNOWN_FEATURE_NUM;
private:
    static uint64_t _f0;
    static uint64_t _f1;
    static Feature *_features[FEATURES_MAX];
    static const char *_names[FEATURES_MAX];
};


#define REGISTER_FEATURE(Feature, num, name) \
    static FeatureRegister __reg_feature(new Feature(), num, name)

static inline std::string featureName(const Feature *const feature)
{
    return FeatureRegister::featureName(feature);
}


#endif // FEATURE_MANAGER_H
