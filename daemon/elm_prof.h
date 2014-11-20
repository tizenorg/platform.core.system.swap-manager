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




struct msg_buf_t;

enum elm_prof_state {
	EPS_ON,
	EPS_OFF
};

int process_msg_elm_prof(struct msg_buf_t *msg_buf);
int elm_prof_set(enum elm_prof_state state);




#ifdef __cplusplus
}
#endif


#endif /* _ELM_PROF_H */
