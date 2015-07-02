/*
 *  DA manager
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Lyupa Anastasia <a.lyupa@samsung.com>
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

#ifndef _UI_VIEWER_DATA_
#define _UI_VIEWER_DATA_

#include <Evas.h>

enum ui_obj_category_t {
	UI_EVAS		= 0x01,
	UI_ELM		= 0x02,
	UI_EDJE		= 0x03
};

enum ui_obj_code_t {
	/* Evas */
	UI_EVAS_UNDEFINED	= 0x0100,
	UI_EVAS_IMAGE		= 0x0101,
	UI_EVAS_LINE		= 0x0102,
	UI_EVAS_POLYGON		= 0X0103,
	UI_EVAS_RECTANGLE	= 0X0104,
	UI_EVAS_TEXT		= 0x0105,
	UI_EVAS_TEXTBLOCK	= 0x0106,
	/* Elementary */
	UI_ELM_UNDEFINED	= 0x0200,
	UI_ELM_BUTTON		= 0x0202,
	UI_ELM_CHECK		= 0x0203,
	UI_ELM_COLORSELECTOR	= 0x0204,
	UI_ELM_CTXPOPUP		= 0x0205,
	UI_ELM_DATETIME		= 0x0206,
	UI_ELM_ENTRY		= 0x0207,
	UI_ELM_FLIP		= 0x0208,
	UI_ELM_GENGRID		= 0x0209,
	UI_ELM_GENLIST		= 0x020A,
	UI_ELM_GLVIEW		= 0x020B,
	UI_ELM_ICON		= 0x020C,
	UI_ELM_IMAGE		= 0x020D,
	UI_ELM_INDEX		= 0x020E,
	UI_ELM_LABEL		= 0x020F,
	UI_ELM_LIST		= 0x0210,
	UI_ELM_MAP		= 0x0211,
	UI_ELM_NOTIFY		= 0x0212,
	UI_ELM_PANEL		= 0x0213,
	UI_ELM_PHOTO		= 0x0214,
	UI_ELM_PHOTOCAM		= 0x0215,
	UI_ELM_PLUG		= 0x0216,
	UI_ELM_POPUP		= 0x0217,
	UI_ELM_PROGRESSBAR	= 0x0218,
	UI_ELM_RADIO		= 0x0219,
	UI_ELM_SEGMENTCONTROL	= 0x021A,
	UI_ELM_SLIDER		= 0x021B,
	UI_ELM_SPINNER		= 0x021C,
	UI_ELM_TOOLBAR		= 0x021D,
	UI_ELM_TOOLTIP		= 0x021E,
	UI_ELM_WIN		= 0x021F,
	/* Edje */
	UI_EDJE_UNDEFINED	= 0x0300,
	UI_EDJE_EDJE		= 0x0301,
	UI_EDJE_BOX		= 0x0302,
	UI_EDJE_DRAG		= 0x0303,
	UI_EDJE_SWALLOW		= 0x0304,
	UI_EDJE_TABLE		= 0x0305,
	UI_EDJE_TEXT		= 0x0306,
	UI_EDJE_TEXTBLOCK	= 0x0307,
	UI_EDJE_RECTANGLE	= 0x0308,
	UI_EDJE_VECTORS		= 0x0309,
	UI_EDJE_BACKGROUND	= 0x0309
};

typedef struct _ui_obj_info_t {
	uint64_t id;
	char category;
	enum ui_obj_code_t code;
	const char *name;
	Eina_Bool redraw;
	uint64_t parent_id;
} ui_obj_info_t;

typedef struct _ui_obj_info_list_t {
	uint32_t count;
	ui_obj_info_t *objs;
} ui_obj_info_list_t;

typedef struct _ui_obj_prop_t {
	uint64_t id;
	char *screenshot;
	uint32_t geometry[4];
	Eina_Bool focus;
	char *name;
	Eina_Bool visible;
	uint32_t color[4];
	Eina_Bool anti_alias;
	float scale;
	uint32_t size_min[2];
	uint32_t size_max[2];
	uint32_t size_request[2];
	float size_align[2];
	float size_weight[2];
	uint32_t size_padding[4];
	Evas_Render_Op render_op;
	char type_prop[0];
} _ui_obj_prop_t;

typedef struct _ui_obj_render_info_t {
	uint64_t id;
	uint64_t parent_id;
	struct timeval tv;
} ui_obj_render_info_t;

typedef struct _ui_obj_render_info_list_t {
	uint32_t count;
	struct ui_obj_render_info_t *objs;
} ui_obj_render_info_list_t;

Eina_List *ui_viewer_get_all_objs(void);
char *pack_string(char *to, const char *str);
char *pack_ui_obj_info_list(char *to, Eina_List *ui_obj_info_list);

static inline char *pack_int8(char *to, uint8_t val)
{
	*(uint8_t *)to = val;
	return to + sizeof(uint8_t);
}

static inline char *pack_int32(char *to, uint32_t val)
{
	*(uint32_t *)to = val;
	return to + sizeof(uint32_t);
}

static inline char *pack_int64(char *to, uint64_t val)
{
	*(uint64_t *)to = val;
	return to + sizeof(uint64_t);
}

#endif /* _UI_VIEWER_DATA_ */
