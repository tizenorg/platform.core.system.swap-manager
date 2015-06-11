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
