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
#include <poll.h>
#include <Elementary.h>

#include "ui_viewer_lib.h"
#include "ui_viewer_utils.h"
#include "ui_viewer_data.h"

static pthread_mutex_t request_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t hierarchy_lock = PTHREAD_MUTEX_INITIALIZER;
enum hierarchy_status_t hierarchy_status = HIERARCHY_NOT_RUNNING;

static Eina_List *_get_obj(Eina_List *obj_list, Evas_Object *obj);
static void pack_ui_obj_prop(int file, Evas_Object *obj, const char *type_name);
static Eina_List *ui_viewer_get_all_objs(void);


static inline void write_int8(int file, uint8_t val)
{
	write(file, &val, sizeof(val));
}

static inline void write_int32(int file, uint32_t val)
{
	write(file, &val, sizeof(val));
}

static inline void write_int64(int file, uint64_t val)
{
	write(file, &val, sizeof(val));
}

static inline void write_float(int file, float val)
{
	write(file, &val, sizeof(val));
}
static inline void write_ptr(int file, const void *val)
{
	uint64_t ptr = (uint64_t)(uintptr_t)val;

	write(file, &ptr, sizeof(ptr));
}

static inline void write_timeval(int file, struct timeval tv)
{
	write_int32(file, tv.tv_sec);
	write_int32(file, tv.tv_usec * 1000);
}

static void write_string(int file, const char *str)
{
	size_t len = strlen(str) + 1;

	write(file, str, len);
}



static enum ui_obj_category_t _get_object_category(const char *type_name)
{
	enum ui_obj_category_t category = UI_UNDEFINED;
	const char evas_prefix[] = "Evas_";
	const char elm_prefix[] = "elm_";

	if (!strcmp(type_name, "rectangle") || !strcmp(type_name, "line") ||
	    !strcmp(type_name, "polygon") || !strcmp(type_name, "text") ||
	    !strcmp(type_name, "textblock") || !strcmp(type_name, "image") ||
	    !strcmp(type_name, "vectors") || !strcmp(type_name, "textgrid") ||
	    !strncmp(type_name, evas_prefix, strlen(evas_prefix)))
		category = UI_EVAS;
	else if (!strncmp(type_name, elm_prefix, strlen(elm_prefix)))
		category = UI_ELM;
	else if (!strcmp(type_name, "edje"))
		category = UI_EDJE;
	else
		ui_viewer_log("cannot get category for type = %s\n", type_name);

	return category;
}

static enum ui_obj_code_t _get_object_type_code(const char *type_name)
{
	enum ui_obj_code_t type_code = UI_CODE_UNDEFINED;
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
	else if (!strcmp(type_name, "vectors"))
		type_code = UI_EVAS_VECTORS;
	else if (!strcmp(type_name, "Evas_Object_Table"))
		type_code = UI_EVAS_TABLE;
	else if (!strcmp(type_name, "Evas_Object_Box"))
		type_code = UI_EVAS_BOX;
	else if (!strcmp(type_name, "textgrid"))
		type_code = UI_EVAS_TEXTGRID;
	else if (!strcmp(type_name, "Evas_Object_Grid"))
		type_code = UI_EVAS_GRID;
	else if (!strcmp(type_name, "Evas_Object_Smart"))
		type_code = UI_EVAS_SMART;
	else if (!strncmp(type_name, evas_prefix, strlen(evas_prefix)))
		type_code = UI_EVAS_UNDEFINED;
	else if (!strcmp(type_name, "elm_bg"))
		type_code = UI_ELM_BACKGROUND;
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
		ui_viewer_log("cannot get type code for type = %s\n", type_name);

	return type_code;
}

static Eina_Bool _get_redraw(enum ui_obj_category_t category)
{
	Eina_Bool redraw = EINA_FALSE;

	if (category != UI_EDJE)
		redraw = EINA_TRUE;

	return redraw;
}

static void _render_obj(Evas __unused *evas, Evas_Object __unused *obj,
			enum ui_obj_category_t category,
			enum ui_obj_code_t type_code EINA_UNUSED,
			struct timeval *tv,
			enum rendering_option_t rendering)
{
	Eina_Bool redraw;

	if ((rendering == RENDER_NONE) ||
	    (rendering == RENDER_NONE_HOST_OPT) ||
	    (rendering == RENDER_ELM && category != UI_ELM)) {
		timerclear(tv);
		return;
	}

	redraw = _get_redraw(category);

	/* TODO FIXME What the hell is going around? This shouldn't be this way */
	if (redraw) {
//		struct timeval start_tv, finish_tv;
//		Eina_Bool visible;
//
//		visible = evas_object_visible_get(obj);
//		if (visible) {
//			evas_object_hide(obj);
//			evas_render(evas);
//		}
//		gettimeofday(&start_tv, NULL);
//		evas_object_show(obj);
//		evas_render(evas);
//		gettimeofday(&finish_tv, NULL);
//		if (!visible) {
//			evas_object_hide(obj);
//			evas_render(evas);
//		}
//		timersub(&finish_tv, &start_tv, tv);
//	} else {
	}
		timerclear(tv);
//	}
}

static Eina_List *_get_obj(Eina_List *obj_list, Evas_Object *obj)
{
	Eina_List *children;
	Evas_Object *child;
	char type_name[MAX_PATH_LENGTH];

	evas_object_ref(obj);
	obj_list = eina_list_append(obj_list, obj);

	_strncpy(type_name, evas_object_type_get(obj), MAX_PATH_LENGTH);

	evas_object_unref(obj);

	if (!strcmp(type_name, "rectangle") || !strcmp(type_name, "line") ||
	    !strcmp(type_name, "polygon") || !strcmp(type_name, "text") ||
	    !strcmp(type_name, "textblock") || !strcmp(type_name, "image") ||
	    !strcmp(type_name, "vectors") || !strcmp(type_name, "textgrid"))
		return obj_list;

	children = evas_object_smart_members_get(obj);
	EINA_LIST_FREE(children, child) {
		obj_list = _get_obj(obj_list, child);
	}
	eina_list_free(children);

	return obj_list;
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

static Eina_Bool _get_obj_exists(Evas_Object *obj)
{
	Eina_Bool exists = EINA_FALSE;

	// this call is safe even if object doesn't exist (returns NULL)
	if (evas_object_type_get(obj) != NULL)
		exists = EINA_TRUE;

	return exists;
}

static Evas_Object *_get_win_id(Evas *evas)
{
	Evas_Object *win_id = 0;
	Eina_List *objs, *l;
	Evas_Object *obj_i;

	objs = evas_objects_in_rectangle_get(evas, SHRT_MIN, SHRT_MIN, USHRT_MAX, USHRT_MAX,
					     EINA_TRUE, EINA_TRUE);
	EINA_LIST_FOREACH(objs, l, obj_i)
	{
		if (!strcmp(evas_object_type_get(obj_i), "elm_win")) {
			win_id = obj_i;
			break;
		}
	}
	eina_list_free(objs);

	return win_id;
}

static void _pack_ui_obj_info(int file, Evas_Object *obj,
			       enum rendering_option_t rendering)
{
	ui_obj_info_t info;
	Evas *evas;

	if (!_get_obj_exists(obj)) {
		ui_viewer_log("object %p doesn't exist\n", obj);
		set_hierarchy_status(HIERARCHY_CANCELLED);
		print_log_ui_viewer_hierarchy_error();
		return;
	}

	evas = evas_object_evas_get(obj);

	info.id = obj;
	_strncpy(info.name, evas_object_type_get(obj), MAX_PATH_LENGTH);
	info.parent_id = evas_object_smart_parent_get(obj);
	info.category = _get_object_category(info.name);
	info.code = _get_object_type_code(info.name);

	// we set window as parent for objects without it
	if (info.parent_id == 0 && strcmp(info.name, "elm_win"))
		info.parent_id = _get_win_id(evas);

	_render_obj(evas, obj, info.category, info.code, &(info.rendering_tv), rendering);

	ui_viewer_log("object : 0x%lx, category : %x, type_code: %x, "
			"type_name : %s, rendering time : %d sec, %d nano sec, parent : 0x%lx\n",
			(unsigned long)info.id, info.category, info.code,
			info.name, info.rendering_tv.tv_sec, info.rendering_tv.tv_usec * 1000,
			(unsigned long)info.parent_id);


	write_ptr(file, info.id);
	write_int8(file, info.category);
	write_int32(file, info.code);
	write_string(file, info.name);
	write_timeval(file, info.rendering_tv);
	write_ptr(file, info.parent_id);
	pack_ui_obj_prop(file, info.id, info.name);

	evas_object_unref(obj);

	return;
}

void pack_ui_obj_info_list(int file, enum rendering_option_t rendering,
			    Eina_Bool *cancelled)
{
	Eina_List *obj_list;
	Evas_Object *obj;
	unsigned int info_cnt;
//	char *start_ptr = to;
	Eina_List *ee_list = NULL, *ee_list_tmp = NULL;
	Ecore_Evas *ee;

	ee_list = ecore_evas_ecore_evas_list_get();
	EINA_LIST_FOREACH(ee_list, ee_list_tmp, ee)
	{
		Evas *evas = ecore_evas_get(ee);
		evas_event_freeze(evas);
	}

	pthread_mutex_lock(&request_lock);
	obj_list = ui_viewer_get_all_objs();
	info_cnt = eina_list_count(obj_list);
	write_int8(file, rendering);
	write_int32(file, info_cnt);
	pthread_mutex_unlock(&request_lock);

	*cancelled = EINA_FALSE;

	if (info_cnt == 0) {
		ui_viewer_log("no objects exist\n");
		set_hierarchy_status(HIERARCHY_CANCELLED);
		*cancelled = EINA_TRUE;
		print_log_ui_viewer_hierarchy_error();
	}

	EINA_LIST_FREE(obj_list, obj)
	{
		pthread_mutex_lock(&request_lock);
		// check if hierarchy request is active
		if (get_hierarchy_status() == HIERARCHY_RUNNING) {
			_pack_ui_obj_info(file, obj, rendering);
		} else {
			ui_viewer_log("break packing hierarchy info\n");
			// don't save any data if request was cancelled
			*cancelled = EINA_TRUE;
			pthread_mutex_unlock(&request_lock);
			break;
		}
		pthread_mutex_unlock(&request_lock);
	}

	pthread_mutex_lock(&request_lock);

	// unref remained objects
	EINA_LIST_FREE(obj_list, obj)
	{
		evas_object_unref(obj);
	}
	eina_list_free(obj_list);

	EINA_LIST_FREE(ee_list, ee)
	{
		Evas *evas = ecore_evas_get(ee);
		evas_event_thaw(evas);
	}
	eina_list_free(ee_list);

	pthread_mutex_unlock(&request_lock);

	return;
}

enum hierarchy_status_t get_hierarchy_status(void)
{
	enum hierarchy_status_t status;

	pthread_mutex_lock(&hierarchy_lock);
	status = hierarchy_status;
	pthread_mutex_unlock(&hierarchy_lock);

	return status;
}

void set_hierarchy_status(enum hierarchy_status_t status)
{
	pthread_mutex_lock(&hierarchy_lock);
	hierarchy_status = status;
	pthread_mutex_unlock(&hierarchy_lock);
}

static Eina_List *_get_all_objs_in_rect(Evas_Coord x, Evas_Coord y, Evas_Coord w,
				 Evas_Coord h,
				 Eina_Bool 	include_pass_events_objects,
				 Eina_Bool 	include_hidden_objects)
{
	Eina_List *ecore_evas_list = NULL, *obj_list = NULL;
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
			obj_list = _get_obj(obj_list, obj_i);
		}
		eina_list_free(objs);
	}

	eina_list_free(ecore_evas_list);

	return obj_list;
}

static Eina_List *ui_viewer_get_all_objs(void)
{
	return _get_all_objs_in_rect(SHRT_MIN, SHRT_MIN, USHRT_MAX, USHRT_MAX,
				    EINA_TRUE, EINA_TRUE);
}

static void _pack_ui_obj_evas_prop(int file, ui_obj_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("evas prop: geometry : [%d,%d,%d,%d], "
			"focus : %d, name : %s, visible : %d, \n\t"
			"color : [%d,%d,%d,%d], anti_alias : %d, scale : %f, "
			"size_min : [%d,%d], size_max : [%d,%d], size_request : [%d,%d], \n\t"
			"size_align : [%f,%f], size_weight : [%f,%f], "
			"size_padding : [%d,%d,%d,%d], render_op : %d\n",
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

	write_int32(file, prop->geometry[0]);
	write_int32(file, prop->geometry[1]);
	write_int32(file, prop->geometry[2]);
	write_int32(file, prop->geometry[3]);
	write_int8(file, prop->focus);
	write_string(file, prop->name);
	write_int8(file, prop->visible);
	write_int32(file, prop->color[0]);
	write_int32(file, prop->color[1]);
	write_int32(file, prop->color[2]);
	write_int32(file, prop->color[3]);
	write_int8(file, prop->anti_alias);
	write_float(file, prop->scale);
	write_int32(file, prop->size_min[0]);
	write_int32(file, prop->size_min[1]);
	write_int32(file, prop->size_max[0]);
	write_int32(file, prop->size_max[1]);
	write_int32(file, prop->size_request[0]);
	write_int32(file, prop->size_request[1]);
	write_float(file, prop->size_align[0]);
	write_float(file, prop->size_align[1]);
	write_float(file, prop->size_weight[0]);
	write_float(file, prop->size_weight[1]);
	write_int32(file, prop->size_padding[0]);
	write_int32(file, prop->size_padding[1]);
	write_int32(file, prop->size_padding[2]);
	write_int32(file, prop->size_padding[3]);
	write_int8(file, prop->render_op);
}

static void _pack_ui_obj_elm_prop(int file, ui_obj_elm_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("elm prop: text : %s, style : %s, disabled : %d, type : %s\n",
		      prop->text, prop->style, prop->disabled, prop->type);

	write_string(file, prop->text);
	write_string(file, prop->style);
	write_int8(file, prop->disabled);
	write_string(file, prop->type);

	return;
}

static void _pack_ui_obj_edje_prop(int file, ui_obj_edje_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("edje prop: animation : %d, play : %d, scale : %f, base_scale : %f, "
		      "size_min : [%d,%d], size_max : [%d,%d]\n",
		      prop->animation, prop->play, prop->scale, prop->base_scale,
		      prop->size_min[0], prop->size_min[1], prop->size_max[0],
		      prop->size_max[1]);

	write_int8(file, prop->animation);
	write_int8(file, prop->play);
	write_float(file, prop->scale);
	write_float(file, prop->base_scale);
	write_int32(file, prop->size_min[0]);
	write_int32(file, prop->size_min[1]);
	write_int32(file, prop->size_max[0]);
	write_int32(file, prop->size_max[1]);
}

static void _pack_image_prop(int file, image_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("image prop: load_dpi : %f, source_clip : %d, filled : %d, content_hint : %d, "
		      "alpha : %d, border : [%d,%d,%d,%d], border_scale : %f, pixels_dirty : %d,\n\t"
		      "load_orientation : %d, border_center_fill : %d, size : [%d,%d], source_visible: %d, "
		      "fill : [%d,%d,%d,%d], load_scale_down : %d,\n\tscale_hint : %d, source_events : %d, "
		      "frame_count : %d, evas_image_stride : %d\n",
		      prop->load_dpi, prop->source_clip, prop->filled, prop->content_hint,
		      prop->alpha, prop->border[0], prop->border[1], prop->border[2],
		      prop->border[3], prop->border_scale, prop->pixels_dirty, prop->load_orientation,
		      prop->border_center_fill, prop->size[0], prop->size[1], prop->source_visible,
		      prop->fill[0], prop->fill[1], prop->fill[2], prop->fill[3], prop->load_scale_down,
		      prop->scale_hint, prop->source_events, prop->frame_count, prop->evas_image_stride);

	write_float(file, prop->load_dpi);
	write_int8(file, prop->source_clip);
	write_int8(file, prop->filled);
	write_int8(file, prop->content_hint);
	write_int8(file, prop->alpha);
	write_int32(file, prop->border[0]);
	write_int32(file, prop->border[1]);
	write_int32(file, prop->border[2]);
	write_int32(file, prop->border[3]);
	write_float(file, prop->border_scale);
	write_int8(file, prop->pixels_dirty);
	write_int8(file, prop->load_orientation);
	write_int8(file, prop->border_center_fill);
	write_int32(file, prop->size[0]);
	write_int32(file, prop->size[1]);
	write_int8(file, prop->source_visible);
	write_int32(file, prop->fill[0]);
	write_int32(file, prop->fill[1]);
	write_int32(file, prop->fill[2]);
	write_int32(file, prop->fill[3]);
	write_int32(file, prop->load_scale_down);
	write_int8(file, prop->scale_hint);
	write_int8(file, prop->source_events);
	write_int32(file, prop->frame_count);
	write_int32(file, prop->evas_image_stride);
}

static void _pack_line_prop(int file, line_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("line prop: xy : [%d,%d,%d,%d]\n",
		      prop->xy[0], prop->xy[1], prop->xy[2], prop->xy[3]);

	write_int32(file, prop->xy[0]);
	write_int32(file, prop->xy[1]);
	write_int32(file, prop->xy[2]);
	write_int32(file, prop->xy[3]);
}

static void _pack_text_prop(int file, text_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("text prop: font : %s, size : %d, text : %s, delim : %s, "
		      "ellipsis : %s, style : %d, shadow_color : [%d,%d,%d,%d], glow_color : [%d,%d,%d,%d],\n\t"
		      "glow2_color : [%d,%d,%d,%d], outline_color : [%d,%d,%d,%d], style_pad: [%d,%d,%d,%d], "
		      "direction : %d\n",
		      prop->font, prop->size, prop->text, prop->delim, prop->ellipsis, prop->style,
		      prop->shadow_color[0], prop->shadow_color[1], prop->shadow_color[2], prop->shadow_color[3],
		      prop->glow_color[0], prop->glow_color[1], prop->glow_color[2], prop->glow_color[3],
		      prop->glow2_color[0], prop->glow2_color[1], prop->glow2_color[2], prop->glow2_color[3],
		      prop->outline_color[0], prop->outline_color[1], prop->outline_color[2], prop->outline_color[3],
		      prop->style_pad[0], prop->style_pad[1], prop->style_pad[2], prop->style_pad[3],
		      prop->direction);

	write_string(file, prop->font);
	write_int32(file, prop->size);
	write_string(file, prop->text);
	write_string(file, prop->delim);
	write_float(file, prop->ellipsis);
	write_int8(file, prop->style);
	write_int32(file, prop->shadow_color[0]);
	write_int32(file, prop->shadow_color[1]);
	write_int32(file, prop->shadow_color[2]);
	write_int32(file, prop->shadow_color[3]);
	write_int32(file, prop->glow_color[0]);
	write_int32(file, prop->glow_color[1]);
	write_int32(file, prop->glow_color[2]);
	write_int32(file, prop->glow_color[3]);
	write_int32(file, prop->glow2_color[0]);
	write_int32(file, prop->glow2_color[1]);
	write_int32(file, prop->glow2_color[2]);
	write_int32(file, prop->glow2_color[3]);
	write_int32(file, prop->outline_color[0]);
	write_int32(file, prop->outline_color[1]);
	write_int32(file, prop->outline_color[2]);
	write_int32(file, prop->outline_color[3]);
	write_int32(file, prop->style_pad[0]);
	write_int32(file, prop->style_pad[1]);
	write_int32(file, prop->style_pad[2]);
	write_int32(file, prop->style_pad[3]);
	write_int8(file, prop->direction);
}

static void _pack_textblock_prop(int file, textblock_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("textblock prop: replace_char : %s, valign : %f, delim : %s, newline : %d, "
		      "markup : %s\n",
		      prop->replace_char, prop->valign, prop->delim, prop->newline,
		      prop->markup);

	write_string(file, prop->replace_char);
	write_float(file, prop->valign);
	write_string(file, prop->delim);
	write_int8(file, prop->newline);
	write_string(file, prop->markup);
}

static void _pack_table_prop(int file, table_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("table prop: homogeneous : %d, align : [%f,%f], padding : [%d,%d], mirrored : %d, "
		      "col_row_size : [%d,%d]\n",
		      prop->homogeneous, prop->align[0], prop->align[1], prop->padding[0],
		      prop->padding[1], prop->mirrored, prop->col_row_size[0], prop->col_row_size[1]);

	write_int8(file, prop->homogeneous);
	write_float(file, prop->align[0]);
	write_float(file, prop->align[1]);
	write_int32(file, prop->padding[0]);
	write_int32(file, prop->padding[1]);
	write_int8(file, prop->mirrored);
	write_int32(file, prop->col_row_size[0]);
	write_int32(file, prop->col_row_size[1]);
}

static void _pack_box_prop(int file, box_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("box prop: align : [%f,%f]\n",
		      prop->align[0], prop->align[1]);

	write_float(file, prop->align[0]);
	write_float(file, prop->align[1]);
}

static void _pack_grid_prop(int file, grid_prop_t *prop)
{
	if (!prop)
		return ;

	ui_viewer_log("grid prop: mirrored : %d\n",
		      prop->mirrored);

	write_int8(file, prop->mirrored);
}

static void _pack_textgrid_prop(int file, textgrid_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("textgrid prop: size : [%d,%d], cell_size : [%d,%d]\n",
		      prop->size[0], prop->size[1], prop->cell_size[0],
		      prop->cell_size[1]);

	write_int32(file, prop->size[0]);
	write_int32(file, prop->size[1]);
	write_int32(file, prop->cell_size[0]);
	write_int32(file, prop->cell_size[1]);
}

static void _pack_bg_prop(int file, bg_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("bg prop: color : [%d,%d,%d], option : %d\n",
		      prop->color[0], prop->color[1], prop->color[2],
		      prop->option);

	write_int32(file, prop->color[0]);
	write_int32(file, prop->color[1]);
	write_int32(file, prop->color[2]);
	write_int8(file, prop->option);
}

static void _pack_button_prop(int file, button_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("button prop: initial_timeout : %f, gap_timeout : %f, autorepeat : %d\n",
		      prop->initial_timeout, prop->gap_timeout, prop->autorepeat);

	write_float(file, prop->initial_timeout);
	write_float(file, prop->gap_timeout);
	write_int8(file, prop->autorepeat);
}

static void _pack_check_prop(int file, check_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("check prop: state : %d\n",
		      prop->state);

	write_int8(file, prop->state);
}

static void _pack_colorselector_prop(int file, colorselector_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("colorselector prop: color : [%d,%d,%d,%d], palette_name : %s, "
		      "mode : %d\n",
		      prop->color[0], prop->color[1], prop->color[2], prop->color[3], prop->palette_name,
		      prop->mode);

	write_int32(file, prop->color[0]);
	write_int32(file, prop->color[1]);
	write_int32(file, prop->color[2]);
	write_int32(file, prop->color[3]);
	write_string(file, prop->palette_name);
	write_int8(file, prop->mode);
}

static void _pack_ctxpopup_prop(int file, ctxpopup_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("ctxpopup prop: horizontal : %d\n",
		      prop->horizontal);

	write_int8(file, prop->horizontal);
}

static void _pack_datetime_prop(int file, datetime_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("datetime prop: format : %s, value : [%d,%d,%d,%d,%d,%d,%d,%d]\n",
		      prop->format, prop->value[0], prop->value[1], prop->value[2], prop->value[3],
		      prop->value[4], prop->value[5], prop->value[6], prop->value[7]);

	write_string(file, prop->format);
	write_int32(file, prop->value[0]);
	write_int32(file, prop->value[1]);
	write_int32(file, prop->value[2]);
	write_int32(file, prop->value[3]);
	write_int32(file, prop->value[4]);
	write_int32(file, prop->value[5]);
	write_int32(file, prop->value[6]);
	write_int32(file, prop->value[7]);
}

static void _pack_entry_prop(int file, entry_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("entry prop: entry : %s, scrollable : %d, panel_show_on_demand : %d, menu_disabled : %d, "
		      "cnp_mode : %d, editable : %d, hover_style : %s, single_line : %d,\n\t"
		      "password : %d, autosave : %d, prediction_allow : %d, panel_enabled: %d, "
		      "cursor_pos : %d, cursor_is_format : %d,\n\tcursor_content : %s, "
		      "selection : %s, is_visible_format : %d\n",
		      prop->entry, prop->scrollable, prop->panel_show_on_demand, prop->menu_disabled,
		      prop->cnp_mode, prop->editable, prop->hover_style, prop->single_line,
		      prop->password, prop->autosave, prop->prediction_allow, prop->panel_enabled,
		      prop->cursor_pos, prop->cursor_is_format, prop->cursor_content, prop->selection,
		      prop->is_visible_format);

	write_string(file, prop->entry);
	write_int8(file, prop->scrollable);
	write_int8(file, prop->panel_show_on_demand);
	write_int8(file, prop->menu_disabled);
	write_int8(file, prop->cnp_mode);
	write_int8(file, prop->editable);
	write_string(file, prop->hover_style);
	write_int8(file, prop->single_line);
	write_int8(file, prop->password);
	write_int8(file, prop->autosave);
	write_int8(file, prop->prediction_allow);
	write_int8(file, prop->panel_enabled);
	write_int32(file, prop->cursor_pos);
	write_int8(file, prop->cursor_is_format);
	write_string(file, prop->cursor_content);
	free(prop->cursor_content); // we need free it according to efl docs
	write_string(file, prop->selection);
	write_int8(file, prop->is_visible_format);
}

static void _pack_flip_prop(int file, flip_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("flip prop: interaction : %d, front_visible : %d\n",
		      prop->interaction, prop->front_visible);

	write_int8(file, prop->interaction);
	write_int8(file, prop->front_visible);
}

static void _pack_gengrid_prop(int file, gengrid_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("gengrid prop: align : [%f,%f], filled : %d, relative : [%f,%f], "
		      "multi_select : %d, group_item_size : [%d,%d], select_mode : %d, render_mode : %d,\n\t"
		      "highlight_mode : %d, item_size : [%d,%d], multi_select_mode: %d, "
		      "horizontal : %d, wheel_disabled : %d, items_count : %d\n",
		      prop->align[0], prop->align[1], prop->filled, prop->relative[0],
		      prop->relative[1], prop->multi_select, prop->group_item_size[0], prop->group_item_size[1],
		      prop->select_mode, prop->render_mode, prop->highlight_mode, prop->item_size[0],
		      prop->item_size[1], prop->multi_select_mode, prop->horizontal, prop->wheel_disabled,
		      prop->items_count);

	write_float(file, prop->align[0]);
	write_float(file, prop->align[1]);
	write_int8(file, prop->filled);
	write_float(file, prop->relative[0]);
	write_float(file, prop->relative[1]);
	write_int8(file, prop->multi_select);
	write_int32(file, prop->group_item_size[0]);
	write_int32(file, prop->group_item_size[1]);
	write_int8(file, prop->select_mode);
	write_int8(file, prop->render_mode);
	write_int8(file, prop->highlight_mode);
	write_int32(file, prop->item_size[0]);
	write_int32(file, prop->item_size[1]);
	write_int8(file, prop->multi_select_mode);
	write_int8(file, prop->horizontal);
	write_int8(file, prop->wheel_disabled);
	write_int32(file, prop->items_count);
}

static void _pack_genlist_prop(int file, genlist_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("genlist prop: multi_select : %d, genlist_mode : %d, items_count : %d, homogeneous : %d, "
		      "block_count : %d, timeout : %f, reorder_mode : %d,\n\t"
		      "decorate_mode : %d, effect_enabled : %d, select_mode: %d, "
		      "highlight_mode : %d, realization_mode : %d\n",
		      prop->multi_select, prop->genlist_mode, prop->items_count, prop->homogeneous,
		      prop->block_count, prop->timeout, prop->reorder_mode, prop->decorate_mode,
		      prop->effect_enabled, prop->select_mode, prop->highlight_mode, prop->realization_mode);

	write_int8(file, prop->multi_select);
	write_int8(file, prop->genlist_mode);
	write_int32(file, prop->items_count);
	write_int8(file, prop->homogeneous);
	write_int32(file, prop->block_count);
	write_float(file, prop->timeout);
	write_int8(file, prop->reorder_mode);
	write_int8(file, prop->decorate_mode);
	write_int8(file, prop->effect_enabled);
	write_int8(file, prop->select_mode);
	write_int8(file, prop->highlight_mode);
	write_int8(file, prop->realization_mode);
}

static void _pack_glview_prop(int file, glview_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("glview prop: size : [%d,%d], rotation: %d\n",
		      prop->size[0], prop->size[1], prop->rotation);

	write_int32(file, prop->size[0]);
	write_int32(file, prop->size[1]);
	write_int32(file, prop->rotation);
}

static void _pack_icon_prop(int file, icon_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("icon prop: lookup : %d, standard : %s\n",
		      prop->lookup, prop->standard);

	write_int8(file, prop->lookup);
	write_string(file, prop->standard);
}

static void _pack_elm_image_prop(int file, elm_image_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("elm_image prop: editable : %d, play : %d, smooth : %d, no_scale : %d, "
		      "animated : %d, aspect_fixed : %d, orient : %d,\n\t"
		      "fill_outside : %d, resizable : [%d,%d], animated_available: %d, "
		      "size : [%d,%d]\n",
		      prop->editable, prop->play, prop->smooth, prop->no_scale,
		      prop->animated, prop->aspect_fixed, prop->orient, prop->fill_outside,
		      prop->resizable[0], prop->resizable[1], prop->animated_available,
		      prop->size[0], prop->size[1]);

	write_int8(file, prop->editable);
	write_int8(file, prop->play);
	write_int8(file, prop->smooth);
	write_int8(file, prop->no_scale);
	write_int8(file, prop->animated);
	write_int8(file, prop->aspect_fixed);
	write_int8(file, prop->orient);
	write_int8(file, prop->fill_outside);
	write_int8(file, prop->resizable[0]);
	write_int8(file, prop->resizable[1]);
	write_int8(file, prop->animated_available);
	write_int32(file, prop->size[0]);
	write_int32(file, prop->size[1]);
}

static void _pack_index_prop(int file, index_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("index prop: autohide_disabled : %d, omit_enabled : %d, priority : %d, horizontal : %d, "
		      "change_time : %f,\n\tindicator_disabled : %d, item_level : %d\n",
		      prop->autohide_disabled, prop->omit_enabled, prop->priority, prop->horizontal,
		      prop->change_time, prop->indicator_disabled, prop->item_level);

	write_int8(file, prop->autohide_disabled);
	write_int8(file, prop->omit_enabled);
	write_int32(file, prop->priority);
	write_int8(file, prop->horizontal);
	write_float(file, prop->change_time);
	write_int8(file, prop->indicator_disabled);
	write_int32(file, prop->item_level);
}

static void _pack_label_prop(int file, label_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("label prop: wrap_width : %d, speed : %f, mode : %d\n",
		      prop->wrap_width, prop->speed, prop->mode);

	write_int32(file, prop->wrap_width);
	write_float(file, prop->speed);
	write_int8(file, prop->mode);
}

static void _pack_list_prop(int file, list_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("list prop: horizontal : %d, select_mode : %d, focus_on_selection : %d, multi_select : %d, "
		      "multi_select_mode : %d, mode : %d\n",
		      prop->horizontal, prop->select_mode, prop->focus_on_selection, prop->multi_select,
		      prop->multi_select_mode, prop->mode);

	write_int8(file, prop->horizontal);
	write_int8(file, prop->select_mode);
	write_int8(file, prop->focus_on_selection);
	write_int8(file, prop->multi_select);
	write_int8(file, prop->multi_select_mode);
	write_int8(file, prop->mode);
}

static void _pack_map_prop(int file, map_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("map prop: zoom : %d, paused : %d, wheel_disabled : %d, zoom_min : %d, "
		      "rotate_degree : %f, rotate : [%d,%d],\n\tagent : %s, zoom_max : %d, "
		      "zoom_mode : %d, region : [%f,%f]\n",
		      prop->zoom, prop->paused, prop->wheel_disabled, prop->zoom_min,
		      prop->rotate_degree, prop->rotate[0], prop->rotate[1],
		      prop->agent, prop->zoom_max, prop->zoom_mode, prop->region[0], prop->region[1]);

	write_int32(file, prop->zoom);
	write_int8(file, prop->paused);
	write_int8(file, prop->wheel_disabled);
	write_int32(file, prop->zoom_min);
	write_float(file, prop->rotate_degree);
	write_int32(file, prop->rotate[0]);
	write_int32(file, prop->rotate[1]);
	write_string(file, prop->agent);
	write_int32(file, prop->zoom_max);
	write_int8(file, prop->zoom_mode);
	write_float(file, prop->region[0]);
	write_float(file, prop->region[1]);
}

static void _pack_notify_prop(int file, notify_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("notify prop: align : [%f,%f], allow_events: %d, "
		      "timeout : %f\n",
		      prop->align[0], prop->align[1], prop->allow_events,
		      prop->timeout);

	write_float(file, prop->align[0]);
	write_float(file, prop->align[1]);
	write_int8(file, prop->allow_events);
	write_float(file, prop->timeout);
}

static void _pack_panel_prop(int file, panel_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("panel prop: orient : %d, hidden : %d, scrollable : %d\n",
		      prop->orient, prop->hidden, prop->scrollable);

	write_int8(file, prop->orient);
	write_int8(file, prop->hidden);
	write_int8(file, prop->scrollable);
}

static void _pack_photo_prop(int file, photo_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("photo prop: editable : %d, fill_inside : %d, aspect_fixed : %d, size : %d\n",
		      prop->editable, prop->fill_inside, prop->aspect_fixed, prop->size);

	write_int8(file, prop->editable);
	write_int8(file, prop->fill_inside);
	write_int8(file, prop->aspect_fixed);
	write_int32(file, prop->size);
}

static void _pack_photocam_prop(int file, photocam_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("photocam prop: paused : %d, file : %f, gesture_enabled : %d, zoom : %f, "
		      "zoom_mode : %d, image_size : [%d,%d]\n",
		      prop->paused, prop->file, prop->gesture_enabled, prop->zoom,
		      prop->zoom_mode, prop->image_size[0], prop->image_size[1]);

	write_int8(file, prop->paused);
	write_string(file, prop->file);
	write_int8(file, prop->gesture_enabled);
	write_float(file, prop->zoom);
	write_int8(file, prop->zoom_mode);
	write_int32(file, prop->image_size[0]);
	write_int32(file, prop->image_size[1]);
}

static void _pack_popup_prop(int file, popup_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("popup prop: align : [%f,%f], allow_events : %d, wrap_type : %d, orient : %d, "
		      "timeout : %f\n",
		      prop->align[0], prop->align[1], prop->allow_events, prop->wrap_type,
		      prop->orient, prop->timeout);

	write_float(file, prop->align[0]);
	write_float(file, prop->align[1]);
	write_int8(file, prop->allow_events);
	write_int8(file, prop->wrap_type);
	write_int8(file, prop->orient);
	write_float(file, prop->timeout);
}

static void _pack_progressbar_prop(int file, progressbar_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("progressbar prop: span_size : %d, pulse : %d, value : %f, inverted : %d, "
		      "horizontal : %d, unit_format : %s\n",
		      prop->span_size, prop->pulse, prop->value, prop->inverted,
		      prop->horizontal, prop->unit_format);

	write_int32(file, prop->span_size);
	write_int8(file, prop->pulse);
	write_float(file, prop->value);
	write_int8(file, prop->inverted);
	write_int8(file, prop->horizontal);
	write_string(file, prop->unit_format);
}

static void _pack_radio_prop(int file, radio_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("radio prop: state_value : %d, value : %d\n",
		      prop->state_value, prop->value);

	write_int32(file, prop->state_value);
	write_int32(file, prop->value);
}

static void _pack_segmencontrol_prop(int file, segmencontrol_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("segmencontrol prop: item_count : %d\n",
		      prop->item_count);

	write_int32(file, prop->item_count);
}

static void _pack_slider_prop(int file, slider_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("slider prop: horizontal : %d, value : %f, format : %s\n",
		      prop->horizontal, prop->value, prop->format);

	write_int8(file, prop->horizontal);
	write_float(file, prop->value);
	write_string(file, prop->format);
}

static void _pack_spinner_prop(int file, spinner_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("spinner prop: min : %f, max : %f, step : %f, wrap : %d, "
		      "interval : %f, round : %d, editable : %d, "
		      "base : %f, value : %f, format : %s\n",
		      prop->min_max[0], prop->min_max[1], prop->step, prop->wrap,
		      prop->interval, prop->round, prop->editable, prop->base,
		      prop->value, prop->format);

	write_float(file, prop->min_max[0]);
	write_float(file, prop->min_max[1]);
	write_float(file, prop->step);
	write_int8(file, prop->wrap);
	write_float(file, prop->interval);
	write_int32(file, prop->round);
	write_int8(file, prop->editable);
	write_float(file, prop->base);
	write_float(file, prop->value);
	write_string(file, prop->format);
}

static void _pack_toolbar_prop(int file, toolbar_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("toolbar prop: reorder_mode : %d, transverse_expanded : %d, homogeneous : %d, align : %f, "
		      "select_mode : %d, icon_size : %d, horizontal : %d,\n\tstandard_priority : %d, "
		      "items_count : %d\n",
		      prop->reorder_mode, prop->transverse_expanded, prop->homogeneous, prop->align,
		      prop->select_mode, prop->icon_size, prop->horizontal, prop->standard_priority,
		      prop->items_count);

	write_int8(file, prop->reorder_mode);
	write_int8(file, prop->transverse_expanded);
	write_int8(file, prop->homogeneous);
	write_float(file, prop->align);
	write_int8(file, prop->select_mode);
	write_int32(file, prop->icon_size);
	write_int8(file, prop->horizontal);
	write_int32(file, prop->standard_priority);
	write_int32(file, prop->items_count);
}

static void _pack_tooltip_prop(int file, tooltip_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("tooltip prop: style : %s, window_mode : %d\n",
		      prop->style, prop->window_mode);

	write_string(file, prop->style);
	write_int8(file, prop->window_mode);
}

static void _pack_win_prop(int file, win_prop_t *prop)
{
	if (!prop)
		return;

	ui_viewer_log("win prop: iconfield : %d, maximized : %d, modal : %d, icon_name : %s, "
		      "withdrawn : %d, role : %s, size_step : [%d,%d], highlight_style : %s,\n\t"
		      "borderless : %d, highlight_enabled : %d, title : %s, alpha: %d, "
		      "urgent : %d, rotation : %d, sticky : %d, highlight_animate : %d,\n\t"
		      "aspect : %f, indicator_opacity : %d, demand_attention : %d, "
		      "layer : %d, profile : %s, shaped : %d, indicator_mode : %d, conformant : %d,\n\t"
		      "size_base : [%d,%d], quickpanel : %d, rotation_supported : %d, screen_dpi : [%d,%d], "
		      "win_type : %d\n",
		      prop->iconfield, prop->maximized, prop->modal, prop->icon_name,
		      prop->withdrawn, prop->role, prop->size_step[0], prop->size_step[1],
		      prop->highlight_style, prop->borderless, prop->highlight_enabled, prop->title,
		      prop->alpha, prop->urgent, prop->rotation, prop->sticky,
		      prop->highlight_animate, prop->aspect, prop->indicator_opacity, prop->demand_attention, prop->layer,
		      prop->profile, prop->shaped, prop->fullscreen, prop->indicator_mode, prop->conformant,
		      prop->size_base[0], prop->size_base[1], prop->quickpanel, prop->rotation_supported, prop->screen_dpi[0],
		      prop->screen_dpi[1], prop->win_type);

	write_int8(file, prop->iconfield);
	write_int8(file, prop->maximized);
	write_int8(file, prop->modal);
	write_string(file, prop->icon_name);
	write_int8(file, prop->withdrawn);
	write_string(file, prop->role);
	write_int32(file, prop->size_step[0]);
	write_int32(file, prop->size_step[1]);
	write_string(file, prop->highlight_style);
	write_int8(file, prop->borderless);
	write_int8(file, prop->highlight_enabled);
	write_string(file, prop->title);
	write_int8(file, prop->alpha);
	write_int8(file, prop->urgent);
	write_int32(file, prop->rotation);
	write_int8(file, prop->sticky);
	write_int8(file, prop->highlight_animate);
	write_float(file, prop->aspect);
	write_int8(file, prop->indicator_opacity);
	write_int8(file, prop->demand_attention);
	write_int32(file, prop->layer);
	write_string(file, prop->profile);
	write_int8(file, prop->shaped);
	write_int8(file, prop->fullscreen);
	write_int8(file, prop->indicator_mode);
	write_int8(file, prop->conformant);
	write_int32(file, prop->size_base[0]);
	write_int32(file, prop->size_base[1]);
	write_int8(file, prop->quickpanel);
	write_int8(file, prop->rotation_supported);
	write_int32(file, prop->screen_dpi[0]);
	write_int32(file, prop->screen_dpi[1]);
	write_int8(file, prop->win_type);
}

static void pack_ui_obj_prop(int file, Evas_Object *obj, const char *type_name)
{
	ui_obj_prop_t obj_prop;
	enum ui_obj_category_t category;
	enum ui_obj_code_t code;

	evas_object_geometry_get(obj, &obj_prop.geometry[0], &obj_prop.geometry[1],
				 &obj_prop.geometry[2], &obj_prop.geometry[3]);
	obj_prop.focus = evas_object_focus_get(obj);
	_strncpy(obj_prop.name, evas_object_name_get(obj), MAX_PATH_LENGTH);
	obj_prop.visible = evas_object_visible_get(obj);
	evas_object_color_get(obj, &obj_prop.color[0], &obj_prop.color[1],
			      &obj_prop.color[2], &obj_prop.color[3]);
	obj_prop.anti_alias = evas_object_anti_alias_get(obj);
	obj_prop.scale = evas_object_scale_get(obj);
	evas_object_size_hint_min_get(obj, &obj_prop.size_min[0], &obj_prop.size_min[1]);
	evas_object_size_hint_max_get(obj, &obj_prop.size_max[0], &obj_prop.size_max[1]);
	evas_object_size_hint_request_get(obj, &obj_prop.size_request[0], &obj_prop.size_request[1]);
	evas_object_size_hint_align_get(obj, &obj_prop.size_align[0], &obj_prop.size_align[1]);
	evas_object_size_hint_weight_get(obj, &obj_prop.size_weight[0], &obj_prop.size_weight[1]);
	evas_object_size_hint_padding_get(obj, &obj_prop.size_padding[0], &obj_prop.size_padding[1],
					  &obj_prop.size_padding[2], &obj_prop.size_padding[3]);
	obj_prop.render_op = evas_object_render_op_get(obj);

	_pack_ui_obj_evas_prop(file, &obj_prop);

	category = _get_object_category(type_name);
	if (category == UI_UNDEFINED) {
		return;
	} else if (category == UI_ELM) {
		ui_obj_elm_prop_t elm_prop;

		if (!strcmp(type_name, "elm_pan")) {
			elm_prop.text[0] = '\0';
			elm_prop.style[0] = '\0';
			elm_prop.disabled = 0;
		} else {
			_strncpy(elm_prop.text, elm_object_text_get(obj), MAX_TEXT_LENGTH);
			_strncpy(elm_prop.style, elm_object_style_get(obj), MAX_PATH_LENGTH);
			elm_prop.disabled = elm_object_disabled_get(obj);
		}
		_strncpy(elm_prop.type, elm_object_widget_type_get(obj), MAX_PATH_LENGTH);

		_pack_ui_obj_elm_prop(file, &elm_prop);

	} else if (category == UI_EDJE) {
		ui_obj_edje_prop_t edje_prop;
		edje_prop.animation = edje_object_animation_get(obj);
		edje_prop.play = edje_object_play_get(obj);
		edje_prop.scale = edje_object_scale_get(obj);
		edje_prop.base_scale = edje_object_base_scale_get(obj);
		edje_object_size_min_get(obj, &edje_prop.size_min[0], &edje_prop.size_min[1]);
		edje_object_size_max_get(obj, &edje_prop.size_max[0], &edje_prop.size_max[1]);

		_pack_ui_obj_edje_prop(file, &edje_prop);
	}

	code = _get_object_type_code(type_name);

	if (code == 0) {
		return;
	} else if (code == UI_EVAS_IMAGE) {
		image_prop_t image_prop;

		image_prop.load_dpi = evas_object_image_load_dpi_get(obj);
		image_prop.source_clip = evas_object_image_source_clip_get(obj);
		image_prop.filled = evas_object_image_filled_get(obj);
		image_prop.content_hint = evas_object_image_content_hint_get(obj);
		image_prop.alpha = evas_object_image_alpha_get(obj);
		evas_object_image_border_get(obj, &(image_prop.border[0]),
					  &(image_prop.border[1]),
					  &(image_prop.border[2]),
					  &(image_prop.border[3]));
		image_prop.border_scale = evas_object_image_border_scale_get(obj);
		image_prop.pixels_dirty = evas_object_image_pixels_dirty_get(obj);
		image_prop.load_orientation = evas_object_image_load_orientation_get(obj);
		image_prop.border_center_fill = evas_object_image_border_center_fill_get(obj);
		evas_object_image_size_get(obj, &(image_prop.size[0]), &(image_prop.size[1]));
		image_prop.source_visible = evas_object_image_source_visible_get(obj);
		evas_object_image_fill_get(obj, &(image_prop.fill[0]),
					&(image_prop.fill[1]),
					&(image_prop.fill[2]),
					&(image_prop.fill[3]));
		image_prop.load_scale_down = evas_object_image_load_scale_down_get(obj);
		image_prop.scale_hint = evas_object_image_scale_hint_get(obj);
		image_prop.source_events = evas_object_image_source_events_get(obj);
		//evas_object_image_animated_frame_count_get(obj); - unstable
		image_prop.frame_count = 0;
		image_prop.evas_image_stride = evas_object_image_stride_get(obj);

		_pack_image_prop(file, &image_prop);
	} else if (code == UI_EVAS_LINE) {
		line_prop_t line_prop;

		evas_object_line_xy_get(obj, &(line_prop.xy[0]),
				     &(line_prop.xy[1]),
				     &(line_prop.xy[2]),
				     &(line_prop.xy[3]));

		_pack_line_prop(file, &line_prop);
	} else if (code == UI_EVAS_TEXT) {
		text_prop_t text_prop;
		const char *text_font;

		evas_object_text_font_get(obj, &text_font, &(text_prop.size));
		_strncpy(text_prop.font, text_font, MAX_PATH_LENGTH);
		_strncpy(text_prop.text, evas_object_text_text_get(obj), MAX_PATH_LENGTH);
		_strncpy(text_prop.delim, evas_object_text_bidi_delimiters_get(obj), MAX_PATH_LENGTH);
		text_prop.ellipsis = evas_object_text_ellipsis_get(obj);
		text_prop.style = evas_object_text_style_get(obj);
		evas_object_text_shadow_color_get(obj, &(text_prop.shadow_color[0]),
					       &(text_prop.shadow_color[1]),
					       &(text_prop.shadow_color[2]),
					       &(text_prop.shadow_color[3]));
		evas_object_text_glow_color_get(obj, &(text_prop.glow_color[0]),
					     &(text_prop.glow_color[1]),
					     &(text_prop.glow_color[2]),
					     &(text_prop.glow_color[3]));
		evas_object_text_glow2_color_get(obj, &(text_prop.glow2_color[0]),
					      &(text_prop.glow2_color[1]),
					      &(text_prop.glow2_color[2]),
					      &(text_prop.glow2_color[3]));
		evas_object_text_outline_color_get(obj, &(text_prop.outline_color[0]),
						&(text_prop.outline_color[1]),
						&(text_prop.outline_color[2]),
						&(text_prop.outline_color[3]));
		evas_object_text_style_pad_get(obj, &(text_prop.style_pad[0]),
					    &(text_prop.style_pad[1]),
					    &(text_prop.style_pad[2]),
					    &(text_prop.style_pad[3]));
		text_prop.direction = evas_object_text_direction_get(obj);

		_pack_text_prop(file, &text_prop);
	} else if (code == UI_EVAS_TEXTBLOCK) {
		textblock_prop_t textblock_prop;

		_strncpy(textblock_prop.replace_char, evas_object_textblock_replace_char_get(obj), MAX_PATH_LENGTH);
		textblock_prop.valign = evas_object_textblock_valign_get(obj);
		_strncpy(textblock_prop.delim, evas_object_textblock_bidi_delimiters_get(obj), MAX_PATH_LENGTH);
		textblock_prop.newline = evas_object_textblock_legacy_newline_get(obj);
		_strncpy(textblock_prop.markup, evas_object_textblock_text_markup_get(obj), MAX_PATH_LENGTH);

		_pack_textblock_prop(file, &textblock_prop);
	} else if (code == UI_EVAS_TABLE) {
		table_prop_t table_prop;

		table_prop.homogeneous = evas_object_table_homogeneous_get(obj);
		evas_object_table_align_get(obj, &(table_prop.align[0]),
					 &(table_prop.align[1]));
		evas_object_table_padding_get(obj, &(table_prop.padding[0]),
					   &(table_prop.padding[1]));
		table_prop.mirrored = evas_object_table_mirrored_get(obj);
		evas_object_table_col_row_size_get(obj, &(table_prop.col_row_size[0]),
						&(table_prop.col_row_size[1]));

		_pack_table_prop(file, &table_prop);
	} else if (code == UI_EVAS_BOX) {
		box_prop_t box_prop;

		evas_object_box_align_get(obj, &(box_prop.align[0]),
				       &(box_prop.align[1]));

		_pack_box_prop(file, &box_prop);
	} else if (code == UI_EVAS_GRID) {
		grid_prop_t grid_prop;

		grid_prop.mirrored = evas_object_grid_mirrored_get(obj);

		_pack_grid_prop(file, &grid_prop);
	} else if (code == UI_EVAS_TEXTGRID) {
		textgrid_prop_t textgrid_prop;

		evas_object_textgrid_size_get(obj, &(textgrid_prop.size[0]),
					   &(textgrid_prop.size[1]));
		evas_object_textgrid_cell_size_get(obj, &(textgrid_prop.cell_size[0]),
						&(textgrid_prop.cell_size[1]));

		_pack_textgrid_prop(file, &textgrid_prop);
	} else if (code == UI_ELM_BACKGROUND) {
		bg_prop_t bg_prop;

		elm_bg_color_get(obj, &(bg_prop.color[0]),
				      &(bg_prop.color[1]),
				      &(bg_prop.color[2]));
		bg_prop.option = elm_bg_option_get(obj);

		_pack_bg_prop(file, &bg_prop);
	} else if (code == UI_ELM_BUTTON) {
		button_prop_t button_prop;

		button_prop.initial_timeout = elm_button_autorepeat_initial_timeout_get(obj);
		button_prop.gap_timeout = elm_button_autorepeat_gap_timeout_get(obj);
		button_prop.autorepeat = elm_button_autorepeat_get(obj);

		_pack_button_prop(file, &button_prop);
	} else if (code == UI_ELM_CHECK) {
		check_prop_t check_prop;

		check_prop.state = elm_check_state_get(obj);

		_pack_check_prop(file, &check_prop);
	} else if (code == UI_ELM_COLORSELECTOR) {
		colorselector_prop_t colorselector_prop;

		elm_colorselector_color_get(obj, &(colorselector_prop.color[0]),
					 &(colorselector_prop.color[1]),
					 &(colorselector_prop.color[2]),
					 &(colorselector_prop.color[3]));
		_strncpy(colorselector_prop.palette_name, elm_colorselector_palette_name_get(obj), MAX_PATH_LENGTH);
		colorselector_prop.mode = elm_colorselector_mode_get(obj);

		_pack_colorselector_prop(file, &colorselector_prop);
	} else if (code == UI_ELM_CTXPOPUP) {
		ctxpopup_prop_t ctxpopup_prop;

		ctxpopup_prop.horizontal = elm_ctxpopup_horizontal_get(obj);

        _pack_ctxpopup_prop(file, &ctxpopup_prop);
	} else if (code == UI_ELM_DATETIME) {
		datetime_prop_t datetime_prop;
		struct tm currtime;

		_strncpy(datetime_prop.format, elm_datetime_format_get(obj), MAX_PATH_LENGTH);
		elm_datetime_value_get(obj, &currtime);
		datetime_prop.value[0] = currtime.tm_sec;
		datetime_prop.value[1] = currtime.tm_min;
		datetime_prop.value[2] = currtime.tm_hour;
		datetime_prop.value[3] = currtime.tm_mday;
		datetime_prop.value[4] = currtime.tm_mon;
		datetime_prop.value[5] = currtime.tm_year;
		datetime_prop.value[6] = currtime.tm_wday;
		datetime_prop.value[7] = currtime.tm_yday;

		_pack_datetime_prop(file, &datetime_prop);
	} else if (code == UI_ELM_ENTRY) {
		entry_prop_t entry_prop;

		_strncpy(entry_prop.entry, elm_entry_entry_get(obj), MAX_PATH_LENGTH);
		entry_prop.scrollable = elm_entry_scrollable_get(obj);
		entry_prop.panel_show_on_demand = elm_entry_input_panel_show_on_demand_get(obj);
		entry_prop.menu_disabled = elm_entry_context_menu_disabled_get(obj);
		entry_prop.cnp_mode = elm_entry_cnp_mode_get(obj);
		entry_prop.editable = elm_entry_editable_get(obj);
		_strncpy(entry_prop.hover_style, elm_entry_anchor_hover_style_get(obj), MAX_PATH_LENGTH);
		entry_prop.single_line= elm_entry_single_line_get(obj);
		entry_prop.password = elm_entry_password_get(obj);
		entry_prop.autosave = elm_entry_autosave_get(obj);
		entry_prop.prediction_allow = elm_entry_prediction_allow_get(obj);
		entry_prop.panel_enabled = elm_entry_input_panel_enabled_get(obj);
		entry_prop.cursor_pos = elm_entry_cursor_pos_get(obj);
		entry_prop.cursor_is_format = elm_entry_cursor_is_format_get(obj);
		entry_prop.cursor_content = elm_entry_cursor_content_get(obj);
		_strncpy(entry_prop.selection, elm_entry_selection_get(obj), MAX_PATH_LENGTH);
		entry_prop.is_visible_format = elm_entry_cursor_is_visible_format_get(obj);

		_pack_entry_prop(file, &entry_prop);
	} else if (code == UI_ELM_FLIP) {
		flip_prop_t flip_prop;

		flip_prop.interaction = elm_flip_interaction_get(obj);
		flip_prop.front_visible = elm_flip_front_visible_get(obj);

		_pack_flip_prop(file, &flip_prop);
	} else if (code == UI_ELM_GENGRID) {
		gengrid_prop_t gengrid_prop;

		elm_gengrid_align_get(obj, &(gengrid_prop.align[0]),
				   &(gengrid_prop.align[1]));
		gengrid_prop.filled = elm_gengrid_filled_get(obj);
		elm_gengrid_page_relative_get(obj, &(gengrid_prop.relative[0]),
					   &(gengrid_prop.relative[1]));
		gengrid_prop.multi_select = elm_gengrid_multi_select_get(obj);
		elm_gengrid_group_item_size_get(obj, &(gengrid_prop.group_item_size[0]),
					     &(gengrid_prop.group_item_size[1]));
		gengrid_prop.select_mode = elm_gengrid_select_mode_get(obj);
		gengrid_prop.render_mode = elm_gengrid_reorder_mode_get(obj);
		gengrid_prop.highlight_mode = elm_gengrid_highlight_mode_get(obj);
		elm_gengrid_item_size_get(obj, &(gengrid_prop.item_size[0]),
				       &(gengrid_prop.item_size[1]));
		gengrid_prop.multi_select_mode = elm_gengrid_multi_select_mode_get(obj);
		gengrid_prop.horizontal = elm_gengrid_horizontal_get(obj);
		gengrid_prop.wheel_disabled = elm_gengrid_wheel_disabled_get(obj);
		gengrid_prop.items_count = elm_gengrid_items_count(obj);

        _pack_gengrid_prop(file, &gengrid_prop);
	} else if (code == UI_ELM_GENLIST) {
		genlist_prop_t genlist_prop;

		genlist_prop.multi_select = elm_genlist_multi_select_get(obj);
		genlist_prop.genlist_mode = elm_genlist_mode_get(obj);
		genlist_prop.items_count = elm_genlist_items_count(obj);
		genlist_prop.homogeneous = elm_genlist_homogeneous_get(obj);
		genlist_prop.block_count = elm_genlist_block_count_get(obj);
		genlist_prop.timeout = elm_genlist_longpress_timeout_get(obj);
		genlist_prop.reorder_mode = elm_genlist_reorder_mode_get(obj);
		genlist_prop.decorate_mode = elm_genlist_decorate_mode_get(obj);
		genlist_prop.effect_enabled = elm_genlist_tree_effect_enabled_get(obj);
		genlist_prop.select_mode = elm_genlist_select_mode_get(obj);
		genlist_prop.highlight_mode = elm_genlist_highlight_mode_get(obj);
		/* TODO: Port this to Tizen 3.0 */
		/* genlist_prop.realization_mode = elm_genlist_realization_mode_get(obj); */
		genlist_prop.realization_mode = 0;

		_pack_genlist_prop(file, &genlist_prop);
	} else if (code == UI_ELM_GLVIEW) {
		glview_prop_t glview_prop;

		elm_glview_size_get(obj, &(glview_prop.size[0]),
				 &(glview_prop.size[1]));
		glview_prop.rotation = elm_glview_rotation_get(obj);

		_pack_glview_prop(file, &glview_prop);
	} else if (code == UI_ELM_ICON) {
		icon_prop_t icon_prop;

		icon_prop.lookup = elm_icon_order_lookup_get(obj);
		_strncpy(icon_prop.standard, elm_icon_standard_get(obj), MAX_PATH_LENGTH);

		_pack_icon_prop(file, &icon_prop);
	} else if (code == UI_ELM_IMAGE) {
		elm_image_prop_t elm_image_prop;

		elm_image_prop.editable = elm_image_editable_get(obj);
		// elm_image_animated_available_get(obj); - unstable
		elm_image_prop.animated_available = EINA_FALSE;
		// elm_image_animated_play_get(obj); - unstable
		elm_image_prop.play = EINA_FALSE;
		elm_image_prop.smooth = elm_image_smooth_get(obj);
		elm_image_prop.no_scale = elm_image_no_scale_get(obj);
		elm_image_prop.animated = elm_image_animated_get(obj);
		elm_image_prop.aspect_fixed = elm_image_aspect_fixed_get(obj);
		elm_image_prop.orient = elm_image_orient_get(obj);
		elm_image_prop.fill_outside = elm_image_fill_outside_get(obj);
		elm_image_resizable_get(obj, &(elm_image_prop.resizable[0]),
				     &(elm_image_prop.resizable[1]));
		elm_image_object_size_get(obj, &(elm_image_prop.size[0]),
				       &(elm_image_prop.size[1]));

		_pack_elm_image_prop(file, &elm_image_prop);
	} else if (code == UI_ELM_INDEX) {
		index_prop_t index_prop;

		index_prop.autohide_disabled = elm_index_autohide_disabled_get(obj);
		index_prop.omit_enabled = elm_index_omit_enabled_get(obj);
		/* TODO: Port this to Tizen 3.0 */
		/* index_prop.priority = elm_index_priority_get(obj); */
		index_prop.horizontal = elm_index_horizontal_get(obj);
		index_prop.change_time = elm_index_delay_change_time_get(obj);
		index_prop.indicator_disabled = elm_index_indicator_disabled_get(obj);
		index_prop.item_level = elm_index_item_level_get(obj);

		_pack_index_prop(file, &index_prop);
	} else if (code == UI_ELM_LABEL) {
		label_prop_t label_prop;

		label_prop.wrap_width = elm_label_wrap_width_get(obj);
		label_prop.speed = elm_label_slide_speed_get(obj);
		label_prop.mode = elm_label_slide_mode_get(obj);

		_pack_label_prop(file, &label_prop);
	} else if (code == UI_ELM_LIST) {
		list_prop_t list_prop;

		list_prop.horizontal = elm_list_horizontal_get(obj);
		list_prop.select_mode = elm_list_select_mode_get(obj);
		list_prop.focus_on_selection = elm_list_focus_on_selection_get(obj);
		list_prop.multi_select = elm_list_multi_select_get(obj);
		list_prop.multi_select_mode = elm_list_multi_select_mode_get(obj);
		list_prop.mode = elm_list_mode_get(obj);

		_pack_list_prop(file, &list_prop);
	} else if (code == UI_ELM_MAP) {
		map_prop_t map_prop;

		map_prop.zoom = elm_map_zoom_get(obj);
		map_prop.paused = elm_map_paused_get(obj);
		map_prop.wheel_disabled = elm_map_wheel_disabled_get(obj);
		map_prop.zoom_min = elm_map_zoom_min_get(obj);
		elm_map_rotate_get(obj, &(map_prop.rotate_degree),
				&(map_prop.rotate[0]),
				&(map_prop.rotate[1]));
		_strncpy(map_prop.agent, elm_map_user_agent_get(obj), MAX_PATH_LENGTH);
		map_prop.zoom_max = elm_map_zoom_max_get(obj);
		map_prop.zoom_mode = elm_map_zoom_mode_get(obj);
		elm_map_region_get(obj, &(map_prop.region[0]),
				&(map_prop.region[1]));

		_pack_map_prop(file, &map_prop);
	} else if (code == UI_ELM_NOTIFY) {
		notify_prop_t notify_prop;

		elm_notify_align_get(obj, &(notify_prop.align[0]),
				  &(notify_prop.align[1]));
		notify_prop.allow_events = elm_notify_allow_events_get(obj);
		notify_prop.timeout = elm_notify_timeout_get(obj);

		_pack_notify_prop(file, &notify_prop);
	} else if (code == UI_ELM_PANEL) {
		panel_prop_t panel_prop;

		panel_prop.orient = elm_panel_orient_get(obj);
		panel_prop.hidden = elm_panel_hidden_get(obj);
		panel_prop.scrollable = elm_panel_scrollable_get(obj);

		_pack_panel_prop(file, &panel_prop);
	} else if (code == UI_ELM_PHOTO) {
		photo_prop_t photo_prop;

		photo_prop.editable = elm_photo_editable_get(obj);
		photo_prop.fill_inside = elm_photo_fill_inside_get(obj);
		photo_prop.aspect_fixed = elm_photo_aspect_fixed_get(obj);
		photo_prop.size = elm_photo_size_get(obj);

		_pack_photo_prop(file, &photo_prop);
	} else if (code == UI_ELM_PHOTOCAM) {
		photocam_prop_t photocam_prop;

		photocam_prop.paused = elm_photocam_paused_get(obj);
		_strncpy(photocam_prop.file, elm_photocam_file_get(obj), MAX_PATH_LENGTH);
		photocam_prop.gesture_enabled = elm_photocam_gesture_enabled_get(obj);
		photocam_prop.zoom = elm_photocam_zoom_get(obj);
		photocam_prop.zoom_mode = elm_photocam_zoom_mode_get(obj);
		elm_photocam_image_size_get(obj, &(photocam_prop.image_size[0]),
					 &(photocam_prop.image_size[1]));

		_pack_photocam_prop(file, &photocam_prop);
	} else if (code == UI_ELM_POPUP) {
		popup_prop_t popup_prop;

		elm_popup_align_get(obj, &(popup_prop.align[0]),
				 &(popup_prop.align[1]));
		popup_prop.allow_events = elm_popup_allow_events_get(obj);
		popup_prop.wrap_type = elm_popup_content_text_wrap_type_get(obj);
		popup_prop.orient = elm_popup_orient_get(obj);
		popup_prop.timeout = elm_popup_timeout_get(obj);

		_pack_popup_prop(file, &popup_prop);
	} else if (code == UI_ELM_PROGRESSBAR) {
		progressbar_prop_t progressbar_prop;

		progressbar_prop.span_size = elm_progressbar_span_size_get(obj);
		progressbar_prop.pulse = elm_progressbar_pulse_get(obj);
		progressbar_prop.value = elm_progressbar_value_get(obj);
		progressbar_prop.inverted = elm_progressbar_inverted_get(obj);
		progressbar_prop.horizontal = elm_progressbar_horizontal_get(obj);
		_strncpy(progressbar_prop.unit_format, elm_progressbar_unit_format_get(obj), MAX_PATH_LENGTH);

		_pack_progressbar_prop(file, &progressbar_prop);
	} else if (code == UI_ELM_RADIO) {
		radio_prop_t radio_prop;

		radio_prop.state_value = elm_radio_state_value_get(obj);
		radio_prop.value = elm_radio_value_get(obj);

		_pack_radio_prop(file, &radio_prop);
	} else if (code == UI_ELM_SEGMENTCONTROL) {
		segmencontrol_prop_t segmencontrol_prop;

		segmencontrol_prop.item_count = elm_segment_control_item_count_get(obj);

		_pack_segmencontrol_prop(file, &segmencontrol_prop);
	} else if (code == UI_ELM_SLIDER) {
		slider_prop_t slider_prop;

		slider_prop.horizontal = elm_slider_horizontal_get(obj);
		slider_prop.value = elm_slider_value_get(obj);
		_strncpy(slider_prop.format, elm_slider_indicator_format_get(obj), MAX_PATH_LENGTH);

		_pack_slider_prop(file, &slider_prop);
	} else if (code == UI_ELM_SPINNER) {
		spinner_prop_t spinner_prop;

		elm_spinner_min_max_get(obj, &(spinner_prop.min_max[0]),
				     &(spinner_prop.min_max[1]));
		spinner_prop.step = elm_spinner_step_get(obj);
		spinner_prop.wrap = elm_spinner_wrap_get(obj);
		spinner_prop.interval = elm_spinner_interval_get(obj);
		spinner_prop.round = elm_spinner_round_get(obj);
		spinner_prop.editable = elm_spinner_editable_get(obj);
		spinner_prop.base = elm_spinner_base_get(obj);
		spinner_prop.value = elm_spinner_value_get(obj);
		_strncpy(spinner_prop.format, elm_spinner_label_format_get(obj), MAX_PATH_LENGTH);

		_pack_spinner_prop(file, &spinner_prop);
	} else if (code == UI_ELM_TOOLBAR) {
		toolbar_prop_t toolbar_prop;

		toolbar_prop.reorder_mode = elm_toolbar_reorder_mode_get(obj);
		toolbar_prop.transverse_expanded = elm_toolbar_transverse_expanded_get(obj);
		toolbar_prop.homogeneous = elm_toolbar_homogeneous_get(obj);
		toolbar_prop.align = elm_toolbar_align_get(obj);
		toolbar_prop.select_mode = elm_toolbar_select_mode_get(obj);
		toolbar_prop.icon_size = elm_toolbar_icon_size_get(obj);
		toolbar_prop.horizontal = elm_toolbar_horizontal_get(obj);
		toolbar_prop.standard_priority = elm_toolbar_standard_priority_get(obj);
		toolbar_prop.items_count = elm_toolbar_items_count(obj);

		_pack_toolbar_prop(file, &toolbar_prop);
	} else if (code == UI_ELM_TOOLTIP) {
		tooltip_prop_t tooltip_prop;

		_strncpy(tooltip_prop.style, elm_object_tooltip_style_get(obj), MAX_PATH_LENGTH);
		tooltip_prop.window_mode = elm_object_tooltip_window_mode_get(obj);

		_pack_tooltip_prop(file, &tooltip_prop);
	} else if (code == UI_ELM_WIN) {
		win_prop_t win_prop;

		win_prop.iconfield = elm_win_iconified_get(obj);
		win_prop.maximized = elm_win_maximized_get(obj);
		win_prop.modal = elm_win_modal_get(obj);
		_strncpy(win_prop.icon_name, elm_win_icon_name_get(obj), MAX_PATH_LENGTH);
		win_prop.withdrawn = elm_win_withdrawn_get(obj);
		_strncpy(win_prop.role, elm_win_role_get(obj), MAX_PATH_LENGTH);
		elm_win_size_step_get(obj, &(win_prop.size_step[0]),
				   &(win_prop.size_step[1]));
		_strncpy(win_prop.highlight_style, elm_win_focus_highlight_style_get(obj), MAX_PATH_LENGTH);
		win_prop.borderless = elm_win_borderless_get(obj);
		win_prop.highlight_enabled = elm_win_focus_highlight_enabled_get(obj);
		_strncpy(win_prop.title, elm_win_title_get(obj), MAX_PATH_LENGTH);
		win_prop.alpha = elm_win_alpha_get(obj);
		win_prop.urgent = elm_win_urgent_get(obj);
		win_prop.rotation = elm_win_rotation_get(obj);
		win_prop.sticky = elm_win_sticky_get(obj);
		win_prop.highlight_animate = elm_win_focus_highlight_animate_get(obj);
		win_prop.aspect = elm_win_aspect_get(obj);
		win_prop.indicator_opacity = elm_win_indicator_opacity_get(obj);
		win_prop.demand_attention = elm_win_demand_attention_get(obj);
		win_prop.layer = elm_win_layer_get(obj);
		_strncpy(win_prop.profile, elm_win_profile_get(obj), MAX_PATH_LENGTH);
		win_prop.shaped = elm_win_shaped_get(obj);
		win_prop.fullscreen = elm_win_fullscreen_get(obj);
		win_prop.indicator_mode = elm_win_indicator_mode_get(obj);
		win_prop.conformant = elm_win_conformant_get(obj);
		elm_win_size_base_get(obj, &(win_prop.size_base[0]),
				   &(win_prop.size_base[1]));
		win_prop.quickpanel = elm_win_quickpanel_get(obj);
		win_prop.rotation_supported = elm_win_wm_rotation_supported_get(obj);
		elm_win_screen_dpi_get(obj, &(win_prop.screen_dpi[0]),
				    &(win_prop.screen_dpi[1]));
		win_prop.win_type = elm_win_type_get(obj);
		_pack_win_prop(file, &win_prop);
	}
}

Eina_Bool _get_shot_in_bg(Evas_Object *obj)
{
	char type_name[MAX_PATH_LENGTH];
	Eina_Bool can_shot_in_bg = EINA_FALSE;

	_strncpy(type_name, evas_object_type_get(obj), MAX_PATH_LENGTH);

	if (!strcmp(type_name, "rectangle") ||
	    !strcmp(type_name, "image") ||
	    !strcmp(type_name, "elm_image") ||
	    !strcmp(type_name, "elm_icon"))
		can_shot_in_bg = EINA_TRUE;

	return can_shot_in_bg;
}

char *pack_ui_obj_screenshot(char *to, Evas_Object *obj)
{
	char screenshot[MAX_PATH_LENGTH];
	enum ErrorCode err_code = ERR_NO;
	Eina_Bool exists;

	screenshot[0] = '\0';

	pthread_mutex_lock(&request_lock);

	exists = _get_obj_exists(obj);

	if (!exists) {
		err_code = ERR_UI_OBJ_NOT_FOUND;
		ui_viewer_log("can't take screenshot of not existing obj = %p\n", obj);
	} else {
		/* Evas_Object *win_id; */
		/* Eina_Bool win_focus; */
		Evas *evas;

		evas_object_ref(obj);
		evas = evas_object_evas_get(obj);
		evas_event_freeze(evas);
		/* win_id = _get_win_id(evas); */
		/* win_focus = elm_win_focus_get(win_id); */

		/* if (win_focus || _get_shot_in_bg(obj)) */
		/* 	ui_viewer_capture_screen(screenshot, obj); */
		/* else */
			err_code = ERR_UI_OBJ_SCREENSHOT_FAILED;
		evas_event_thaw(evas);
		evas_object_unref(obj);
	}

	ui_viewer_log("screenshot: obj : %p, err_code : %d, path : %s\n", obj, err_code, screenshot);

	to = pack_int32(to, err_code);
	to = pack_string(to, screenshot);

	pthread_mutex_unlock(&request_lock);

	return to;
}
