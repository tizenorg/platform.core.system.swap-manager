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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <Elementary.h>

#include "ui_viewer_lib.h"
#include "ui_viewer_utils.h"
#include "ui_viewer_data.h"


static Eina_List *_get_obj_info(Eina_List *ui_obj_info_list, Evas_Object *obj);

static char _get_object_category(Evas_Object *obj)
{
	char category = 0;
	const char *type_name = evas_object_type_get(obj);
	const char evas_prefix[] = "Evas_";
	const char elm_prefix[] = "elm_";

	if (!strcmp(type_name, "rectangle") || !strcmp(type_name, "line") ||
	    !strcmp(type_name, "polygon") || !strcmp(type_name, "text") ||
	    !strcmp(type_name, "textblock") || !strcmp(type_name, "image") ||
	    !strncmp(type_name, evas_prefix, strlen(evas_prefix)))
		category = UI_EVAS;
	else if (!strncmp(type_name, elm_prefix, strlen(elm_prefix)))
		category = UI_ELM;
	else if (!strcmp(type_name, "edje"))
		category = UI_EDJE;
	else
		fprintf(stderr, "cannot get category for obj = %lx, type = %s\n",
			(unsigned long) obj, type_name);

	return category;
}

static int _get_object_type_code(Evas_Object *obj)
{
	int type_code = 0;
	const char *type_name = evas_object_type_get(obj);
	const char evas_prefix[] = "Evas_";
	const char elm_prefix[] = "elm_";

	if (!strcmp(type_name, "rectangle"))
		type_code = UI_EVAS_RECTANGLE;
	else if (!strcmp(type_name, "line"))
		type_code = UI_EVAS_LINE;
	else if (!strcmp(type_name, "polygon"))
		type_code = UI_EVAS_POLYGON;
	else if (!strcmp(type_name, "text"))
		type_code = UI_EVAS_TEXT;
	else if (!strcmp(type_name, "textblock"))
		type_code = UI_EVAS_TEXTBLOCK;
	else if (!strcmp(type_name, "image"))
		type_code = UI_EVAS_IMAGE;
	else if (!strncmp(type_name, evas_prefix, strlen(evas_prefix)))
		type_code = UI_EVAS_UNDEFINED;
	else if (!strcmp(type_name, "elm_button"))
		type_code = UI_ELM_BUTTON;
	else if (!strcmp(type_name, "elm_check"))
		type_code = UI_ELM_CHECK;
	else if (!strcmp(type_name, "elm_colorselector"))
		type_code = UI_ELM_COLORSELECTOR;
	else if (!strcmp(type_name, "elm_ctxpopup"))
		type_code = UI_ELM_CTXPOPUP;
	else if (!strcmp(type_name, "elm_datetime"))
		type_code = UI_ELM_DATETIME;
	else if (!strcmp(type_name, "elm_entry"))
		type_code = UI_ELM_ENTRY;
	else if (!strcmp(type_name, "elm_flip"))
		type_code = UI_ELM_FLIP;
	else if (!strcmp(type_name, "elm_gengrid"))
		type_code = UI_ELM_GENGRID;
	else if (!strcmp(type_name, "elm_genlist"))
		type_code = UI_ELM_GENLIST;
	else if (!strcmp(type_name, "elm_glview"))
		type_code = UI_ELM_GLVIEW;
	else if (!strcmp(type_name, "elm_icon"))
		type_code = UI_ELM_ICON;
	else if (!strcmp(type_name, "elm_image"))
		type_code = UI_ELM_IMAGE;
	else if (!strcmp(type_name, "elm_index"))
		type_code = UI_ELM_INDEX;
	else if (!strcmp(type_name, "elm_label"))
		type_code = UI_ELM_LABEL;
	else if (!strcmp(type_name, "elm_list"))
		type_code = UI_ELM_LIST;
	else if (!strcmp(type_name, "elm_map"))
		type_code = UI_ELM_MAP;
	else if (!strcmp(type_name, "elm_notify"))
		type_code = UI_ELM_NOTIFY;
	else if (!strcmp(type_name, "elm_panel"))
		type_code = UI_ELM_PANEL;
	else if (!strcmp(type_name, "elm_photo"))
		type_code = UI_ELM_PHOTO;
	else if (!strcmp(type_name, "elm_photocam"))
		type_code = UI_ELM_PHOTOCAM;
	else if (!strcmp(type_name, "elm_plug"))
		type_code = UI_ELM_PLUG;
	else if (!strcmp(type_name, "elm_popup"))
		type_code = UI_ELM_POPUP;
	else if (!strcmp(type_name, "elm_progressbar"))
		type_code = UI_ELM_PROGRESSBAR;
	else if (!strcmp(type_name, "elm_radio"))
		type_code = UI_ELM_RADIO;
	else if (!strcmp(type_name, "elm_segment_control"))
		type_code = UI_ELM_SEGMENTCONTROL;
	else if (!strcmp(type_name, "elm_slider"))
		type_code = UI_ELM_SLIDER;
	else if (!strcmp(type_name, "elm_spinner"))
		type_code = UI_ELM_SPINNER;
	else if (!strcmp(type_name, "elm_toolbar"))
		type_code = UI_ELM_TOOLBAR;
	else if (!strcmp(type_name, "elm_tooltip"))
		type_code = UI_ELM_TOOLTIP;
	else if (!strcmp(type_name, "elm_win"))
		type_code = UI_ELM_WIN;
	else if (!strncmp(type_name, elm_prefix, strlen(elm_prefix)))
		type_code = UI_ELM_UNDEFINED;
	else if (!strcmp(type_name, "edje"))
		type_code = UI_EDJE_UNDEFINED;
	else
		fprintf(stderr, "cannot get type code for obj = %lx, type = %s\n",
			(unsigned long) obj, type_name);

	return type_code;
}

static Eina_List *_get_obj_info(Eina_List *ui_obj_info_list, Evas_Object *obj)
{
	Eina_List *children;
	Evas_Object *child;
	ui_obj_info_t *info;

	info = (ui_obj_info_t*)malloc(sizeof(ui_obj_info_t));

	if (!info) {
		fprintf(stderr, "Cannot alloc ui_obj_info_t: %d bytes\n",
			 sizeof(*info));
		return NULL;
	}

	fprintf(stderr, "object_id : %lx, object_category : %d, object_type_code: %d, "
			"object_type_name : %s, object_possible_redraw : %d, "
			"parent_object_id : %lx\n",
			(unsigned long) obj,
			_get_object_category(obj),
			_get_object_type_code(obj),
			evas_object_type_get(obj),
			elm_object_widget_check(obj),
			(unsigned long) evas_object_smart_parent_get(obj));

	info->id = obj;
	info->category = _get_object_category(obj);
	info->code = _get_object_type_code(obj);
	info->name = evas_object_type_get(obj);
	info->redraw = elm_object_widget_check(obj);
	info->parent_id = evas_object_smart_parent_get(obj);

	ui_obj_info_list = eina_list_append(ui_obj_info_list, info);

	children = evas_object_smart_members_get(obj);
	EINA_LIST_FREE(children, child) {
		ui_obj_info_list = _get_obj_info(ui_obj_info_list, child);
	}
	eina_list_free(children);

	return ui_obj_info_list;
}

static void _correct_parent_ids(Eina_List *ui_obj_info_list)
{
	Eina_List *tmp_list;
	ui_obj_info_t *info;
	Evas_Object *win_id = 0;

	EINA_LIST_REVERSE_FOREACH(ui_obj_info_list, tmp_list, info) {
		if (!strcmp(info->name, "elm_win")) {
			win_id = info->id;
			break;
		}
	}

	if (!win_id)
		fprintf(stderr, "Cannot find elm window\n");

	EINA_LIST_FOREACH(ui_obj_info_list, tmp_list, info) {
		if (strcmp(info->name, "elm_win") && info->parent_id == 0)
			info->parent_id = win_id;
	}
}

char *pack_string(char *to, const char *str)
{
	if (!str) {
		*to = '\0';
		return to + 1;
	} else {
		size_t len = strlen(str) + 1;
		strncpy(to, str, len);
		return to + len;
	}
}

static char *_pack_ui_obj_info(char *to, ui_obj_info_t *info)
{
	to = pack_ptr(to, info->id);
	to = pack_int8(to, info->category);
	to = pack_int32(to, info->code);
	to = pack_string(to, info->name);
	to = pack_int8(to, info->redraw);
	to = pack_ptr(to, info->parent_id);

	return to;
}

char *pack_ui_obj_info_list(char *to, Eina_List *ui_obj_info_list)
{
	ui_obj_info_t *info_i;
	unsigned int info_cnt = eina_list_count(ui_obj_info_list);

	to = pack_int32(to, info_cnt);

	EINA_LIST_FREE(ui_obj_info_list, info_i)
	{
		to = _pack_ui_obj_info(to, info_i);
	}
	eina_list_free(ui_obj_info_list);

	return to;
}

static Eina_List *_get_all_objs_in_rect(Evas_Coord x, Evas_Coord y, Evas_Coord w,
				 Evas_Coord h,
				 Eina_Bool 	include_pass_events_objects,
				 Eina_Bool 	include_hidden_objects)
{
	Eina_List *ecore_evas_list = NULL, *ui_obj_info_list = NULL;
	Ecore_Evas *ee;

	ecore_evas_list = ecore_evas_ecore_evas_list_get();

	EINA_LIST_FREE(ecore_evas_list, ee)
	{
		Evas *evas;
		Eina_List *objs, *l;
		Evas_Object *obj_i;

		evas = ecore_evas_get(ee);

		objs = evas_objects_in_rectangle_get(evas, x, y, w, h,
						     include_pass_events_objects,
						     include_hidden_objects);

		EINA_LIST_FOREACH(objs, l, obj_i)
		{
			ui_obj_info_list = _get_obj_info(ui_obj_info_list, obj_i);
		}
		eina_list_free(objs);

		_correct_parent_ids(ui_obj_info_list);
	}

	eina_list_free(ecore_evas_list);

	return ui_obj_info_list;
}

Eina_List *ui_viewer_get_all_objs(void)
{
	return _get_all_objs_in_rect(SHRT_MIN, SHRT_MIN, USHRT_MAX, USHRT_MAX,
				    EINA_TRUE, EINA_TRUE);
}

static Eina_Bool _check_obj_children(Evas_Object *cur_obj, Evas_Object *obj);

static Eina_Bool _check_obj_children(Evas_Object *cur_obj, Evas_Object *obj)
{
	Eina_List *children;
	Evas_Object *child;
	Eina_Bool exists = EINA_FALSE;

	if (cur_obj == obj)
		return EINA_TRUE;

	children = evas_object_smart_members_get(cur_obj);
	EINA_LIST_FREE(children, child) {
		if (!exists)
			exists = _check_obj_children(child, obj);
	}
	eina_list_free(children);

	return EINA_FALSE;
}

static Eina_Bool _check_obj_exists(Evas_Object *obj)
{
	Eina_List *ecore_evas_list = NULL;
	Ecore_Evas *ee;
	Eina_Bool exists = EINA_FALSE;

	ecore_evas_list = ecore_evas_ecore_evas_list_get();

	EINA_LIST_FREE(ecore_evas_list, ee)
	{
		Evas *evas;
		Eina_List *objs, *l;
		Evas_Object *obj_i;

		evas = ecore_evas_get(ee);

		objs = evas_objects_in_rectangle_get(evas, SHRT_MIN, SHRT_MIN,
						     USHRT_MAX, USHRT_MAX,
						     EINA_TRUE, EINA_TRUE);

		EINA_LIST_FOREACH(objs, l, obj_i)
		{
			exists = _check_obj_children(obj_i, obj);
		}
		eina_list_free(objs);
	}

	eina_list_free(ecore_evas_list);

	return exists;
}

char *pack_ui_obj_prop(char *to, ui_obj_prop_t *prop)
{
	to = pack_ptr(to, prop->id);
	to = pack_string(to, prop->screenshot);
	to = pack_int32(to, prop->geometry[0]);
	to = pack_int32(to, prop->geometry[1]);
	to = pack_int32(to, prop->geometry[2]);
	to = pack_int32(to, prop->geometry[3]);
	to = pack_int8(to, prop->focus);
	to = pack_string(to, prop->name);
	to = pack_int8(to, prop->visible);
	to = pack_int32(to, prop->color[0]);
	to = pack_int32(to, prop->color[1]);
	to = pack_int32(to, prop->color[2]);
	to = pack_int32(to, prop->color[3]);
	to = pack_int8(to, prop->anti_alias);
	to = pack_float(to, prop->scale);
	to = pack_int32(to, prop->size_min[0]);
	to = pack_int32(to, prop->size_min[1]);
	to = pack_int32(to, prop->size_max[0]);
	to = pack_int32(to, prop->size_max[1]);
	to = pack_int32(to, prop->size_request[0]);
	to = pack_int32(to, prop->size_request[1]);
	to = pack_float(to, prop->size_align[0]);
	to = pack_float(to, prop->size_align[1]);
	to = pack_float(to, prop->size_weight[0]);
	to = pack_float(to, prop->size_weight[1]);
	to = pack_int32(to, prop->size_padding[0]);
	to = pack_int32(to, prop->size_padding[1]);
	to = pack_int32(to, prop->size_padding[2]);
	to = pack_int32(to, prop->size_padding[3]);
	to = pack_int8(to, prop->render_op);

	return to;
}

ui_obj_prop_t *ui_viewer_get_object_properties(Evas_Object *obj)
{
	ui_obj_prop_t *prop;
	Eina_Bool exists;
	Evas_Coord coord[4];
	int color[4];
	double x, y;

	exists = _check_obj_exists(obj);

	fprintf(stderr, "get_object_properties for 0x%lx, exists: %d\n",
		(unsigned long)obj, exists);

	if (!exists)
		return NULL;

	prop = (ui_obj_prop_t*)malloc(sizeof(ui_obj_prop_t));

	if (!prop) {
		fprintf(stderr, "Cannot alloc ui_obj_prop_t: %d bytes\n",
			 sizeof(*prop));
		return NULL;
	}

	prop->id = obj;
	prop->screenshot = NULL;
	evas_object_geometry_get(obj, &coord[0], &coord[1],
				 &coord[2], &coord[3]);
	prop->geometry[0] = (uint32_t)coord[0];
	prop->geometry[1] = (uint32_t)coord[1];
	prop->geometry[2] = (uint32_t)coord[2];
	prop->geometry[3] = (uint32_t)coord[3];
	prop->focus = evas_object_focus_get(obj);
	prop->name = evas_object_name_get(obj);
	prop->visible = evas_object_visible_get(obj);
	evas_object_color_get(obj, &color[0], &color[1],
			      &color[2], &color[3]);
	prop->color[0] = (uint32_t)color[0];
	prop->color[1] = (uint32_t)color[1];
	prop->color[2] = (uint32_t)color[2];
	prop->color[3] = (uint32_t)color[3];
	prop->anti_alias = evas_object_anti_alias_get(obj);
	prop->scale = evas_object_scale_get(obj);
	evas_object_size_hint_min_get(obj, &coord[0], &coord[1]);
	prop->size_min[0] = (uint32_t)coord[0];
	prop->size_min[1] = (uint32_t)coord[1];
	evas_object_size_hint_max_get(obj, &coord[0], &coord[1]);
	prop->size_max[0] = (uint32_t)coord[0];
	prop->size_max[1] = (uint32_t)coord[1];
	evas_object_size_hint_request_get(obj, &coord[0], &coord[1]);
	prop->size_request[0] = (uint32_t)coord[0];
	prop->size_request[1] = (uint32_t)coord[1];
	evas_object_size_hint_align_get(obj, &x, &y);
	prop->size_align[0] = (float)x;
	prop->size_align[1] = (float)y;
	evas_object_size_hint_weight_get(obj, &x, &y);
	prop->size_weight[0] = (float)x;
	prop->size_weight[1] = (float)y;
	evas_object_size_hint_padding_get(obj, &coord[0], &coord[1],
					  &coord[2], &coord[3]);
	prop->size_padding[0] = (uint32_t)coord[0];
	prop->size_padding[1] = (uint32_t)coord[1];
	prop->size_padding[2] = (uint32_t)coord[2];
	prop->size_padding[3] = (uint32_t)coord[3];
	prop->render_op = evas_object_render_op_get(obj);
	prop->type_prop = NULL;

	fprintf(stderr, "object_id : %lx, screenshot : %s, geometry: [%d,%d,%d,%d], "
			"focus : %d, name : %s, visible : %d, \n"
			"color : [%d,%d,%d,%d], anti_alias : %d, scale : %f, "
			"size_min : [%d,%d], size_max : [%d,%d], size_request : [%d,%d], \n"
			"size_align : [%f,%f], size_weight : [%f,%f], "
			"size_padding : [%d,%d,%d,%d], render_op : %d\n",
			(unsigned long)prop->id, prop->screenshot,
			prop->geometry[0], prop->geometry[1],
			prop->geometry[2], prop->geometry[3],
			prop->focus, prop->name, prop->visible,
			prop->color[0], prop->color[1],
			prop->color[2], prop->color[3],
			prop->anti_alias, prop->scale,
			prop->size_min[0], prop->size_min[1],
			prop->size_max[0], prop->size_max[1],
			prop->size_request[0], prop->size_request[1],
			prop->size_align[0], prop->size_align[1],
			prop->size_weight[0], prop->size_weight[1],
			prop->size_padding[0], prop->size_padding[1],
			prop->size_padding[2], prop->size_padding[3],
			prop->render_op);

	return prop;
}

Eina_List *ui_viewer_get_rendering_time(Evas_Object *obj)
{
	Eina_Bool exists;

	exists = _check_obj_exists(obj);

	fprintf(stderr, "get rendering time for 0x%lx, exists: %d\n",
		(unsigned long)obj, exists);

	if (!exists)
		return NULL;

	if (!elm_object_widget_check(obj))
		return NULL;

	return NULL;
}
