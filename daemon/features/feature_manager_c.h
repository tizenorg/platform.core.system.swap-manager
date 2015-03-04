#ifndef FEATURE_MANAGER_C_H
#define FEATURE_MANAGER_C_H

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


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>


int fm_init(void);
void fm_uninit(void);

int fm_start(void);
int fm_stop(void);

int fm_set(uint64_t f0, uint64_t f1);


#ifdef __cplusplus
}
#endif


#endif // FEATURE_MANAGER_C_H
