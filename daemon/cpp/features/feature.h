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

#ifndef FEATURE_H
#define FEATURE_H


#include <mutex>
#include <string>


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
#define val2bit(val) (size_t)max_bit<(val)>::num


class Feature
{
public:
    Feature();
    virtual ~Feature();

    int init();
    int uninit();

    int enable();
    int disable();

private:
    std::string name();
    int virtual doInit() { return 0; }
    int virtual doUninit() { return 0; }

    int virtual doEnable() { return 0; }
    int virtual doDisable() { return 0; }

    enum Flag {
        F_INIT,
        F_ENABLE,
        F_UNINIT
    };

    enum Flag _flag;
    std::mutex _mutex;
};

#endif // FEATURE_H
