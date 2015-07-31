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


#include "AppInstCont.h"
#include "AppInst.h"


AppInstCont &AppInstCont::instance()
{
    static AppInstCont cont;

    return cont;
}

AppInstCont::AppInstCont()
{
}

AppInstCont::~AppInstCont()
{
    clear();
}

int AppInstCont::add(AppType type, const std::string &id,
                     const std::string &path, const ByteArray &data)
{
    AppInstInfo info(id, path, data);

    AppInst *app = AppInst::create(type, info);
    if (app == 0)
        return -EINVAL;

    _list.push_back(app);

    return 0;
}

void AppInstCont::clear()
{
    for (AppInstList::const_iterator it = _list.begin(); it != _list.end(); ++it)
        AppInst::destroy(*it);

    _list.clear();
}
