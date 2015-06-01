#ifndef FEATURE_MANAGER_H
#define FEATURE_MANAGER_H

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

    static const size_t UNKNOWN_FEATURE_NUM;
private:
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
