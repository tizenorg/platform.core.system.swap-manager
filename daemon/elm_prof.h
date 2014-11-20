#ifndef _ELM_PROF_H
#define _ELM_PROF_H

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
 * 2014         Vyacheslav Cherkashin <v.cherkashin@samsung.com>
 *
 */


#ifdef __cplusplus
extern "C" {
#endif




int elm_prof_add(const char *app_path, unsigned long main_addr,
		 const char *lib_path, unsigned long appcore_addr);
void elm_prof_rm(const char *path);
void elm_prof_rm_all(void);




#ifdef __cplusplus
}
#endif


#endif /* _ELM_PROF_H */
