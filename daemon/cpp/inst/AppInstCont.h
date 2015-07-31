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


#ifndef APPINSTCONT_H
#define APPINSTCONT_H


#include <list>
#include "AppInst.h"


class AppInstCont
{
public:
    static AppInstCont &instance();

    int add(AppType type, const std::string &id, const std::string &path, const ByteArray &data);
    void clear();

    template <typename T, typename Data>
    void for_each(void (*func)(const T&, Data&), Data& data)
    {
        for (AppInstList::const_iterator it = _list.begin(); it != _list.end(); ++it) {
            T *app = dynamic_cast<T*>(*it);
            if (app)
                func(*app, data);
        }
    }

private:
    AppInstCont();
    ~AppInstCont();

    typedef std::list<AppInst *> AppInstList;

    AppInstList _list;
};


#endif // APPINSTCONT_H
