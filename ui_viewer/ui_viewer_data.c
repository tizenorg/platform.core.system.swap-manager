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

static enum ui_obj_category_t _get_object_category(Evas_Object *obj)
{
	char category = 0;
	const char *type_name = evas_object_type_get(obj);
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
		ui_viewer_log("cannot get category for obj = %lx, type = %s\n",
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
		ui_viewer_log("cannot get type code for obj = %lx, type = %s\n",
			(unsigned long) obj, type_name);

	return type_code;
}

static Eina_Bool _get_redraw(Evas_Object *obj)
{
	Eina_Bool redraw = EINA_FALSE;
	const char *type =  evas_object_type_get(obj);

	if (strcmp(type, "edje") &&
	    strcmp(type, "elm_win") &&
	    strcmp(type, "elm_access") &&
	    strcmp(type, "elm_gesture_layer"))
		redraw = EINA_TRUE;

	return redraw;
}

static Eina_List *_get_obj_info(Eina_List *ui_obj_info_list, Evas_Object *obj)
{
	Eina_List *children;
	Evas_Object *child;
	ui_obj_info_t *info;

	info = (ui_obj_info_t*)malloc(sizeof(ui_obj_info_t));

	if (!info) {
		ui_viewer_log("Cannot alloc ui_obj_info_t: %d bytes\n",
			 sizeof(ui_obj_info_t));
		return NULL;
	}

	info->id = obj;
	info->category = _get_object_category(obj);
	info->code = _get_object_type_code(obj);
	info->name = evas_object_type_get(obj);
	info->redraw = _get_redraw(obj);
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
		ui_viewer_log("Cannot find elm window\n");

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
	ui_viewer_log("object : 0x%lx, category : %x, type_code: %x, "
			"type_name : %s, redraw : %d, parent : 0x%lx\n",
			(unsigned long)info->id, info->category, info->code,
			info->name, info->redraw, (unsigned long)info->parent_id);

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

	return exists;
}

static Eina_Bool _check_obj_exists(Evas_Object *obj)
{
	Eina_List *ecore_evas_list = NULL;
	Ecore_Evas *ee;
	Eina_Bool exists = EINA_FALSE;

	ecore_evas_list = ecore_evas_ecore_evas_list_get();

	EINA_LIST_FREE(ecore_evas_list, ee)
	{
		if (!exists) {
			Evas *evas;
			Eina_List *objs, *l;
			Evas_Object *obj_i;

			evas = ecore_evas_get(ee);

			objs = evas_objects_in_rectangle_get(evas, SHRT_MIN, SHRT_MIN,
							     USHRT_MAX, USHRT_MAX,
							     EINA_TRUE, EINA_TRUE);

			EINA_LIST_FOREACH(objs, l, obj_i)
			{
				if (!exists)
					exists = _check_obj_children(obj_i, obj);
			}
			eina_list_free(objs);
		}
	}

	eina_list_free(ecore_evas_list);

	return exists;
}

static char *_pack_ui_obj_evas_prop(char *to, ui_obj_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("object_id : 0x%lx, screenshot : %s, geometry : [%d,%d,%d,%d], "
			"focus : %d, name : %s, visible : %d, \n\t"
			"color : [%d,%d,%d,%d], anti_alias : %d, scale : %f, "
			"size_min : [%d,%d], size_max : [%d,%d], size_request : [%d,%d], \n\t"
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

static char *_pack_ui_obj_elm_prop(char *to, ui_obj_elm_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("elm prop: text : %s, style : %s, disabled : %d, type : %s\n",
		      prop->text, prop->style, prop->disabled, prop->type);

	to = pack_string(to, prop->text);
	to = pack_string(to, prop->style);
	to = pack_int8(to, prop->disabled);
	to = pack_string(to, prop->type);

	return to;
}

static char *_pack_ui_obj_edje_prop(char *to, ui_obj_edje_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("edje prop: animation : %d, play : %d, scale : %f, base_scale : %f, "
		      "size_min : [%d,%d], size_max : [%d,%d]\n",
		      prop->animation, prop->play, prop->scale, prop->base_scale,
		      prop->size_min[0], prop->size_min[1], prop->size_max[0],
		      prop->size_max[1]);

	to = pack_int8(to, prop->animation);
	to = pack_int8(to, prop->play);
	to = pack_float(to, prop->scale);
	to = pack_float(to, prop->base_scale);
	to = pack_int32(to, prop->size_min[0]);
	to = pack_int32(to, prop->size_min[1]);
	to = pack_int32(to, prop->size_max[0]);
	to = pack_int32(to, prop->size_max[1]);

	return to;
}

static char *_pack_image_prop(char *to, image_prop_t *prop)
{
	if (!prop)
		return to;

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

	to = pack_float(to, prop->load_dpi);
	to = pack_int8(to, prop->source_clip);
	to = pack_int8(to, prop->filled);
	to = pack_int8(to, prop->content_hint);
	to = pack_int8(to, prop->alpha);
	to = pack_int32(to, prop->border[0]);
	to = pack_int32(to, prop->border[1]);
	to = pack_int32(to, prop->border[2]);
	to = pack_int32(to, prop->border[3]);
	to = pack_float(to, prop->border_scale);
	to = pack_int8(to, prop->pixels_dirty);
	to = pack_int8(to, prop->load_orientation);
	to = pack_int8(to, prop->border_center_fill);
	to = pack_int32(to, prop->size[0]);
	to = pack_int32(to, prop->size[1]);
	to = pack_int8(to, prop->source_visible);
	to = pack_int32(to, prop->fill[0]);
	to = pack_int32(to, prop->fill[1]);
	to = pack_int32(to, prop->fill[2]);
	to = pack_int32(to, prop->fill[3]);
	to = pack_int32(to, prop->load_scale_down);
	to = pack_int8(to, prop->scale_hint);
	to = pack_int8(to, prop->source_events);
	to = pack_int32(to, prop->frame_count);
	to = pack_int32(to, prop->evas_image_stride);

	return to;
}

static char *_pack_line_prop(char *to, line_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("line prop: xy : [%d,%d,%d,%d]\n",
		      prop->xy[0], prop->xy[1], prop->xy[2], prop->xy[3]);

	to = pack_int32(to, prop->xy[0]);
	to = pack_int32(to, prop->xy[1]);
	to = pack_int32(to, prop->xy[2]);
	to = pack_int32(to, prop->xy[3]);

	return to;
}

static char *_pack_text_prop(char *to, text_prop_t *prop)
{
	if (!prop)
		return to;

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

	to = pack_string(to, prop->font);
	to = pack_int32(to, prop->size);
	to = pack_string(to, prop->text);
	to = pack_string(to, prop->delim);
	to = pack_float(to, prop->ellipsis);
	to = pack_int8(to, prop->style);
	to = pack_int32(to, prop->shadow_color[0]);
	to = pack_int32(to, prop->shadow_color[1]);
	to = pack_int32(to, prop->shadow_color[2]);
	to = pack_int32(to, prop->shadow_color[3]);
	to = pack_int32(to, prop->glow_color[0]);
	to = pack_int32(to, prop->glow_color[1]);
	to = pack_int32(to, prop->glow_color[2]);
	to = pack_int32(to, prop->glow_color[3]);
	to = pack_int32(to, prop->glow2_color[0]);
	to = pack_int32(to, prop->glow2_color[1]);
	to = pack_int32(to, prop->glow2_color[2]);
	to = pack_int32(to, prop->glow2_color[3]);
	to = pack_int32(to, prop->outline_color[0]);
	to = pack_int32(to, prop->outline_color[1]);
	to = pack_int32(to, prop->outline_color[2]);
	to = pack_int32(to, prop->outline_color[3]);
	to = pack_int32(to, prop->style_pad[0]);
	to = pack_int32(to, prop->style_pad[1]);
	to = pack_int32(to, prop->style_pad[2]);
	to = pack_int32(to, prop->style_pad[3]);
	to = pack_int8(to, prop->direction);

	return to;
}

static char *_pack_textblock_prop(char *to, textblock_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("textblock prop: replace_char : %s, valign : %f, delim : %s, newline : %d, "
		      "markup : %s\n",
		      prop->replace_char, prop->valign, prop->delim, prop->newline,
		      prop->markup);

	to = pack_string(to, prop->replace_char);
	to = pack_float(to, prop->valign);
	to = pack_string(to, prop->delim);
	to = pack_int8(to, prop->newline);
	to = pack_string(to, prop->markup);

	return to;
}

static char *_pack_table_prop(char *to, table_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("table prop: homogeneous : %d, align : [%f,%f], padding : [%d,%d], mirrored : %d, "
		      "col_row_size : [%d,%d]\n",
		      prop->homogeneous, prop->align[0], prop->align[1], prop->padding[0],
		      prop->padding[1], prop->mirrored, prop->col_row_size[0], prop->col_row_size[1]);

	to = pack_int8(to, prop->homogeneous);
	to = pack_float(to, prop->align[0]);
	to = pack_float(to, prop->align[1]);
	to = pack_int32(to, prop->padding[0]);
	to = pack_int32(to, prop->padding[1]);
	to = pack_int8(to, prop->mirrored);
	to = pack_int32(to, prop->col_row_size[0]);
	to = pack_int32(to, prop->col_row_size[1]);

	return to;
}

static char *_pack_box_prop(char *to, box_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("box prop: align : [%f,%f], padding : [%d,%d]\n",
		      prop->align[0], prop->align[1], prop->padding[0],
		      prop->padding[1]);

	to = pack_float(to, prop->align[0]);
	to = pack_float(to, prop->align[1]);
	to = pack_int32(to, prop->padding[0]);
	to = pack_int32(to, prop->padding[1]);

	return to;
}

static char *_pack_grid_prop(char *to, grid_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("grid prop: mirrored : %d\n",
		      prop->mirrored);

	to = pack_int8(to, prop->mirrored);

	return to;
}

static char *_pack_textgrid_prop(char *to, textgrid_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("textgrid prop: size : [%d,%d], cell_size : [%d,%d]\n",
		      prop->size[0], prop->size[1], prop->cell_size[0],
		      prop->cell_size[1]);

	to = pack_int32(to, prop->size[0]);
	to = pack_int32(to, prop->size[1]);
	to = pack_int32(to, prop->cell_size[0]);
	to = pack_int32(to, prop->cell_size[1]);

	return to;
}

static char *_pack_bg_prop(char *to, bg_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("bg prop: color : [%d,%d,%d], option : %d\n",
		      prop->color[0], prop->color[1], prop->color[2],
		      prop->option);

	to = pack_int32(to, prop->color[0]);
	to = pack_int32(to, prop->color[1]);
	to = pack_int32(to, prop->color[2]);
	to = pack_int8(to, prop->option);

	return to;
}

static char *_pack_button_prop(char *to, button_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("button prop: initial_timeout : %f, gap_timeout : %f, autorepeat : %d\n",
		      prop->initial_timeout, prop->gap_timeout, prop->autorepeat);

	to = pack_float(to, prop->initial_timeout);
	to = pack_float(to, prop->gap_timeout);
	to = pack_int8(to, prop->autorepeat);

	return to;
}

static char *_pack_check_prop(char *to, check_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("check prop: state : %d\n",
		      prop->state);

	to = pack_int8(to, prop->state);

	return to;
}

static char *_pack_colorselector_prop(char *to, colorselector_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("colorselector prop: color : [%d,%d,%d,%d], palette_name : %s, "
		      "mode : %d\n",
		      prop->color[0], prop->color[1], prop->color[2], prop->color[3], prop->palette_name,
		      prop->mode);

	to = pack_int32(to, prop->color[0]);
	to = pack_int32(to, prop->color[1]);
	to = pack_int32(to, prop->color[2]);
	to = pack_int32(to, prop->color[3]);
	to = pack_string(to, prop->palette_name);
	to = pack_int8(to, prop->mode);

	return to;
}

static char *_pack_ctxpopup_prop(char *to, ctxpopup_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("ctxpopup prop: horizontal : %d\n",
		      prop->horizontal);

	to = pack_int8(to, prop->horizontal);

	return to;
}

static char *_pack_datetime_prop(char *to, datetime_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("datetime prop: format : %s, value : [%d,%d,%d,%d,%d,%d,%d,%d]\n",
		      prop->format, prop->value[0], prop->value[1], prop->value[2], prop->value[3],
		      prop->value[4], prop->value[5], prop->value[6], prop->value[7]);

	to = pack_string(to, prop->format);
	to = pack_int32(to, prop->value[0]);
	to = pack_int32(to, prop->value[1]);
	to = pack_int32(to, prop->value[2]);
	to = pack_int32(to, prop->value[3]);
	to = pack_int32(to, prop->value[4]);
	to = pack_int32(to, prop->value[5]);
	to = pack_int32(to, prop->value[6]);
	to = pack_int32(to, prop->value[7]);

	return to;
}

static char *_pack_entry_prop(char *to, entry_prop_t *prop)
{
	if (!prop)
		return to;

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

	to = pack_string(to, prop->entry);
	to = pack_int8(to, prop->scrollable);
	to = pack_int8(to, prop->panel_show_on_demand);
	to = pack_int8(to, prop->menu_disabled);
	to = pack_int8(to, prop->cnp_mode);
	to = pack_int8(to, prop->editable);
	to = pack_string(to, prop->hover_style);
	to = pack_int8(to, prop->single_line);
	to = pack_int8(to, prop->password);
	to = pack_int8(to, prop->autosave);
	to = pack_int8(to, prop->prediction_allow);
	to = pack_int8(to, prop->panel_enabled);
	to = pack_int32(to, prop->cursor_pos);
	to = pack_int8(to, prop->cursor_is_format);
	to = pack_string(to, prop->cursor_content);
	free(prop->cursor_content); // we need free it according to efl docs
	to = pack_string(to, prop->selection);
	to = pack_int8(to, prop->is_visible_format);

	return to;
}

static char *_pack_flip_prop(char *to, flip_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("flip prop: interaction : %d, front_visible : %d\n",
		      prop->interaction, prop->front_visible);

	to = pack_int8(to, prop->interaction);
	to = pack_int8(to, prop->front_visible);

	return to;
}

static char *_pack_gengrid_prop(char *to, gengrid_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("gengrid prop: align : [%f,%f], filled : %d, relative : [%f,%f], "
		      "multi_select : %d, group_item_size : [%d,%d], select_mode : %d, render_mode : %d,\n\t"
		      "highlight_mode : %d, item_size : [%d,%d], multi_select_mode: %d, "
		      "horizontal : %d, wheel_disabled : %d, items_count : %d\n",
		      prop->align[0], prop->align[1], prop->filled, prop->relative[0],
		      prop->relative[1], prop->multi_select, prop->group_item_size[0], prop->group_item_size[1],
		      prop->select_mode, prop->render_mode, prop->highlight_mode, prop->item_size[0],
		      prop->item_size[1], prop->multi_select_mode, prop->horizontal, prop->wheel_disabled,
		      prop->items_count);

	to = pack_float(to, prop->align[0]);
	to = pack_float(to, prop->align[1]);
	to = pack_int8(to, prop->filled);
	to = pack_float(to, prop->relative[0]);
	to = pack_float(to, prop->relative[1]);
	to = pack_int8(to, prop->multi_select);
	to = pack_int32(to, prop->group_item_size[0]);
	to = pack_int32(to, prop->group_item_size[1]);
	to = pack_int8(to, prop->select_mode);
	to = pack_int8(to, prop->render_mode);
	to = pack_int8(to, prop->highlight_mode);
	to = pack_int32(to, prop->item_size[0]);
	to = pack_int32(to, prop->item_size[1]);
	to = pack_int8(to, prop->multi_select_mode);
	to = pack_int8(to, prop->horizontal);
	to = pack_int8(to, prop->wheel_disabled);
	to = pack_int32(to, prop->items_count);

	return to;
}

static char *_pack_genlist_prop(char *to, genlist_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("genlist prop: multi_select : %d, genlist_mode : %d, items_count : %d, homogeneous : %d, "
		      "block_count : %d, timeout : %f, reorder_mode : %d,\n\t"
		      "decorate_mode : %d, effect_enabled : %d, select_mode: %d, "
		      "highlight_mode : %d, realization_mode : %d\n",
		      prop->multi_select, prop->genlist_mode, prop->items_count, prop->homogeneous,
		      prop->block_count, prop->timeout, prop->reorder_mode, prop->decorate_mode,
		      prop->effect_enabled, prop->select_mode, prop->highlight_mode, prop->realization_mode);

	to = pack_int8(to, prop->multi_select);
	to = pack_int8(to, prop->genlist_mode);
	to = pack_int32(to, prop->items_count);
	to = pack_int8(to, prop->homogeneous);
	to = pack_int32(to, prop->block_count);
	to = pack_float(to, prop->timeout);
	to = pack_int8(to, prop->reorder_mode);
	to = pack_int8(to, prop->decorate_mode);
	to = pack_int8(to, prop->effect_enabled);
	to = pack_int8(to, prop->select_mode);
	to = pack_int8(to, prop->highlight_mode);
	to = pack_int8(to, prop->realization_mode);

	return to;
}

static char *_pack_glview_prop(char *to, glview_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("glview prop: size : [%d,%d], rotation: %d\n",
		      prop->size[0], prop->size[1], prop->rotation);

	to = pack_int32(to, prop->size[0]);
	to = pack_int32(to, prop->size[1]);
	to = pack_int32(to, prop->rotation);

	return to;
}

static char *_pack_icon_prop(char *to, icon_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("icon prop: lookup : %d, standard : %s\n",
		      prop->lookup, prop->standard);

	to = pack_int8(to, prop->lookup);
	to = pack_string(to, prop->standard);

	return to;
}

static char *_pack_elm_image_prop(char *to, elm_image_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("elm_image prop: editable : %d, play : %d, smooth : %d, no_scale : %d, "
		      "animated : %d, aspect_fixed : %d, orient : %d,\n\t"
		      "fill_outside : %d, resizable : [%d,%d], animated_available: %d, "
		      "size : [%d,%d]\n",
		      prop->editable, prop->play, prop->smooth, prop->no_scale,
		      prop->animated, prop->aspect_fixed, prop->orient, prop->fill_outside,
		      prop->resizable[0], prop->resizable[1], prop->animated_available,
		      prop->size[0], prop->size[1]);

	to = pack_int8(to, prop->editable);
	to = pack_int8(to, prop->play);
	to = pack_int8(to, prop->smooth);
	to = pack_int8(to, prop->no_scale);
	to = pack_int8(to, prop->animated);
	to = pack_int8(to, prop->aspect_fixed);
	to = pack_int8(to, prop->orient);
	to = pack_int8(to, prop->fill_outside);
	to = pack_int8(to, prop->resizable[0]);
	to = pack_int8(to, prop->resizable[1]);
	to = pack_int8(to, prop->animated_available);
	to = pack_int32(to, prop->size[0]);
	to = pack_int32(to, prop->size[1]);

	return to;
}

static char *_pack_index_prop(char *to, index_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("index prop: autohide_disabled : %d, omit_enabled : %d, priority : %d, horizontal : %d, "
		      "change_time : %f,\n\tindicator_disabled : %d, item_level : %d\n",
		      prop->autohide_disabled, prop->omit_enabled, prop->priority, prop->horizontal,
		      prop->change_time, prop->indicator_disabled, prop->item_level);

	to = pack_int8(to, prop->autohide_disabled);
	to = pack_int8(to, prop->omit_enabled);
	to = pack_int32(to, prop->priority);
	to = pack_int8(to, prop->horizontal);
	to = pack_float(to, prop->change_time);
	to = pack_int8(to, prop->indicator_disabled);
	to = pack_int32(to, prop->item_level);

	return to;
}

static char *_pack_label_prop(char *to, label_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("label prop: wrap_width : %d, speed : %f, mode : %d\n",
		      prop->wrap_width, prop->speed, prop->mode);

	to = pack_int32(to, prop->wrap_width);
	to = pack_float(to, prop->speed);
	to = pack_int8(to, prop->mode);

	return to;
}

static char *_pack_list_prop(char *to, list_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("list prop: horizontal : %d, select_mode : %d, focus_on_selection : %d, multi_select : %d, "
		      "multi_select_mode : %d, mode : %d\n",
		      prop->horizontal, prop->select_mode, prop->focus_on_selection, prop->multi_select,
		      prop->multi_select_mode, prop->mode);

	to = pack_int8(to, prop->horizontal);
	to = pack_int8(to, prop->select_mode);
	to = pack_int8(to, prop->focus_on_selection);
	to = pack_int8(to, prop->multi_select);
	to = pack_int8(to, prop->multi_select_mode);
	to = pack_int8(to, prop->mode);

	return to;
}

static char *_pack_map_prop(char *to, map_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("map prop: zoom : %d, paused : %d, wheel_disabled : %d, zoom_min : %d, "
		      "rotate_degree : %f, rotate : [%d,%d],\n\tagent : %s, zoom_max : %d, "
		      "zoom_mode : %d, region : [%f,%f]\n",
		      prop->zoom, prop->paused, prop->wheel_disabled, prop->zoom_min,
		      prop->rotate_degree, prop->rotate[0], prop->rotate[1],
		      prop->agent, prop->zoom_max, prop->zoom_mode, prop->region[0], prop->region[1]);

	to = pack_int32(to, prop->zoom);
	to = pack_int8(to, prop->paused);
	to = pack_int8(to, prop->wheel_disabled);
	to = pack_int32(to, prop->zoom_min);
	to = pack_float(to, prop->rotate_degree);
	to = pack_int32(to, prop->rotate[0]);
	to = pack_int32(to, prop->rotate[1]);
	to = pack_string(to, prop->agent);
	to = pack_int32(to, prop->zoom_max);
	to = pack_int8(to, prop->zoom_mode);
	to = pack_float(to, prop->region[0]);
	to = pack_float(to, prop->region[1]);

	return to;
}

static char *_pack_notify_prop(char *to, notify_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("notify prop: align : [%f,%f], allow_events: %d, "
		      "timeout : %f\n",
		      prop->align[0], prop->align[1], prop->allow_events,
		      prop->timeout);

	to = pack_float(to, prop->align[0]);
	to = pack_float(to, prop->align[1]);
	to = pack_int8(to, prop->allow_events);
	to = pack_float(to, prop->timeout);

	return to;
}

static char *_pack_panel_prop(char *to, panel_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("panel prop: orient : %d, hidden : %d, scrollable : %d\n",
		      prop->orient, prop->hidden, prop->scrollable);

	to = pack_int8(to, prop->orient);
	to = pack_int8(to, prop->hidden);
	to = pack_int8(to, prop->scrollable);

	return to;
}

static char *_pack_photo_prop(char *to, photo_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("photo prop: editable : %d, fill_inside : %d, aspect_fixed : %d, size : %d\n",
		      prop->editable, prop->fill_inside, prop->aspect_fixed, prop->size);

	to = pack_int8(to, prop->editable);
	to = pack_int8(to, prop->fill_inside);
	to = pack_int8(to, prop->aspect_fixed);
	to = pack_int32(to, prop->size);

	return to;
}

static char *_pack_photocam_prop(char *to, photocam_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("photocam prop: paused : %d, file : %f, gesture_enabled : %d, zoom : %f, "
		      "zoom_mode : %d, image_size : [%d,%d]\n",
		      prop->paused, prop->file, prop->gesture_enabled, prop->zoom,
		      prop->zoom_mode, prop->image_size[0], prop->image_size[1]);

	to = pack_int8(to, prop->paused);
	to = pack_string(to, prop->file);
	to = pack_int8(to, prop->gesture_enabled);
	to = pack_float(to, prop->zoom);
	to = pack_int8(to, prop->zoom_mode);
	to = pack_int32(to, prop->image_size[0]);
	to = pack_int32(to, prop->image_size[1]);

	return to;
}

static char *_pack_popup_prop(char *to, popup_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("popup prop: align : [%f,%f], allow_events : %d, wrap_type : %d, orient : %d, "
		      "timeout : %f\n",
		      prop->align[0], prop->align[1], prop->allow_events, prop->wrap_type,
		      prop->orient, prop->timeout);

	to = pack_float(to, prop->align[0]);
	to = pack_float(to, prop->align[1]);
	to = pack_int8(to, prop->allow_events);
	to = pack_int8(to, prop->wrap_type);
	to = pack_int8(to, prop->orient);
	to = pack_float(to, prop->timeout);

	return to;
}

static char *_pack_progressbar_prop(char *to, progressbar_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("progressbar prop: span_size : %d, pulse : %d, value : %f, inverted : %d, "
		      "horizontal : %d, unit_format : %s\n",
		      prop->span_size, prop->pulse, prop->value, prop->inverted,
		      prop->horizontal, prop->unit_format);

	to = pack_int32(to, prop->span_size);
	to = pack_int8(to, prop->pulse);
	to = pack_float(to, prop->value);
	to = pack_int8(to, prop->inverted);
	to = pack_int8(to, prop->horizontal);
	to = pack_string(to, prop->unit_format);

	return to;
}

static char *_pack_radio_prop(char *to, radio_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("radio prop: state_value : %d, value : %d\n",
		      prop->state_value, prop->value);

	to = pack_int32(to, prop->state_value);
	to = pack_int32(to, prop->value);

	return to;
}

static char *_pack_segmencontrol_prop(char *to, segmencontrol_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("segmencontrol prop: item_count : %d\n",
		      prop->item_count);

	to = pack_int32(to, prop->item_count);

	return to;
}

static char *_pack_slider_prop(char *to, slider_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("slider prop: horizontal : %d, value : %f, format : %s\n",
		      prop->horizontal, prop->value, prop->format);

	to = pack_int8(to, prop->horizontal);
	to = pack_float(to, prop->value);
	to = pack_string(to, prop->format);

	return to;
}

static char *_pack_spinner_prop(char *to, spinner_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("spinner prop: min : %f, max : %f, step : %f, wrap : %d, "
		      "interval : %f, round : %d, editable : %d, "
		      "base : %f, value : %f, format : %s\n",
		      prop->min_max[0], prop->min_max[1], prop->step, prop->wrap,
		      prop->interval, prop->round, prop->editable, prop->base,
		      prop->value, prop->format);

	to = pack_float(to, prop->min_max[0]);
	to = pack_float(to, prop->min_max[1]);
	to = pack_float(to, prop->step);
	to = pack_int8(to, prop->wrap);
	to = pack_float(to, prop->interval);
	to = pack_int32(to, prop->round);
	to = pack_int8(to, prop->editable);
	to = pack_float(to, prop->base);
	to = pack_float(to, prop->value);
	to = pack_string(to, prop->format);

	return to;
}

static char *_pack_toolbar_prop(char *to, toolbar_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("toolbar prop: reorder_mode : %d, transverse_expanded : %d, homogeneous : %d, align : %f, "
		      "select_mode : %d, icon_size : %d, horizontal : %d,\n\tstandard_priority : %d, "
		      "items_count : %d\n",
		      prop->reorder_mode, prop->transverse_expanded, prop->homogeneous, prop->align,
		      prop->select_mode, prop->icon_size, prop->horizontal, prop->standard_priority,
		      prop->items_count);

	to = pack_int8(to, prop->reorder_mode);
	to = pack_int8(to, prop->transverse_expanded);
	to = pack_int8(to, prop->homogeneous);
	to = pack_float(to, prop->align);
	to = pack_int8(to, prop->select_mode);
	to = pack_int32(to, prop->icon_size);
	to = pack_int8(to, prop->horizontal);
	to = pack_int32(to, prop->standard_priority);
	to = pack_int32(to, prop->items_count);

	return to;
}

static char *_pack_tooltip_prop(char *to, tooltip_prop_t *prop)
{
	if (!prop)
		return to;

	ui_viewer_log("tooltip prop: style : %s, window_mode : %d\n",
		      prop->style, prop->window_mode);

	to = pack_string(to, prop->style);
	to = pack_int8(to, prop->window_mode);

	return to;
}

static char *_pack_win_prop(char *to, win_prop_t *prop)
{
	if (!prop)
		return to;

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

	to = pack_int8(to, prop->iconfield);
	to = pack_int8(to, prop->maximized);
	to = pack_int8(to, prop->modal);
	to = pack_string(to, prop->icon_name);
	to = pack_int8(to, prop->withdrawn);
	to = pack_string(to, prop->role);
	to = pack_int32(to, prop->size_step[0]);
	to = pack_int32(to, prop->size_step[1]);
	to = pack_string(to, prop->highlight_style);
	to = pack_int8(to, prop->borderless);
	to = pack_int8(to, prop->highlight_enabled);
	to = pack_string(to, prop->title);
	to = pack_int8(to, prop->alpha);
	to = pack_int8(to, prop->urgent);
	to = pack_int32(to, prop->rotation);
	to = pack_int8(to, prop->sticky);
	to = pack_int8(to, prop->highlight_animate);
	to = pack_float(to, prop->aspect);
	to = pack_int8(to, prop->indicator_opacity);
	to = pack_int8(to, prop->demand_attention);
	to = pack_int32(to, prop->layer);
	to = pack_string(to, prop->profile);
	to = pack_int8(to, prop->shaped);
	to = pack_int8(to, prop->fullscreen);
	to = pack_int8(to, prop->indicator_mode);
	to = pack_int8(to, prop->conformant);
	to = pack_int32(to, prop->size_base[0]);
	to = pack_int32(to, prop->size_base[1]);
	to = pack_int8(to, prop->quickpanel);
	to = pack_int8(to, prop->rotation_supported);
	to = pack_int32(to, prop->screen_dpi[0]);
	to = pack_int32(to, prop->screen_dpi[1]);
	to = pack_int8(to, prop->win_type);

	return to;
}

char *pack_ui_obj_prop(char *to, Evas_Object *obj)
{
	ui_obj_prop_t obj_prop;
	Eina_Bool exists;
	enum ui_obj_category_t category;
	enum ui_obj_code_t code;

	if (!obj)
		return to;

	exists = _check_obj_exists(obj);

	ui_viewer_log("get_object_properties for 0x%lx, exists: %d\n",
		(unsigned long)obj, exists);

	if (!exists)
		return to;

	obj_prop.id = obj;
	if (captureScreen(obj_prop.screenshot) != 0)
		obj_prop.screenshot[0] = '\0';
	evas_object_geometry_get(obj, &obj_prop.geometry[0], &obj_prop.geometry[1],
				 &obj_prop.geometry[2], &obj_prop.geometry[3]);
	obj_prop.focus = evas_object_focus_get(obj);
	obj_prop.name = evas_object_name_get(obj);
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

	to = _pack_ui_obj_evas_prop(to, &obj_prop);

	category = _get_object_category(obj);

	if (category == UI_ELM) {
		ui_obj_elm_prop_t elm_prop;
		elm_prop.text = elm_object_text_get(obj);
		elm_prop.style = elm_object_style_get(obj);
		elm_prop.disabled = elm_object_disabled_get(obj);
		elm_prop.type = elm_object_widget_type_get(obj);

		to = _pack_ui_obj_elm_prop(to, &elm_prop);

	} else if (category == UI_EDJE) {
		ui_obj_edje_prop_t edje_prop;
		edje_prop.animation = edje_object_animation_get(obj);
		edje_prop.play = edje_object_play_get(obj);
		edje_prop.scale = edje_object_scale_get(obj);
		edje_prop.base_scale = edje_object_base_scale_get(obj);
		edje_object_size_min_get(obj, &edje_prop.size_min[0], &edje_prop.size_min[1]);
		edje_object_size_max_get(obj, &edje_prop.size_max[0], &edje_prop.size_max[1]);

		to = _pack_ui_obj_edje_prop(to, &edje_prop);
	}

	code = _get_object_type_code(obj);

	switch (code) {
		image_prop_t image_prop;
		line_prop_t line_prop;
		text_prop_t text_prop;
		textblock_prop_t textblock_prop;
		table_prop_t table_prop;
		box_prop_t box_prop;
		grid_prop_t grid_prop;
		textgrid_prop_t textgrid_prop;
		bg_prop_t bg_prop;
		button_prop_t button_prop;
		check_prop_t check_prop;
		colorselector_prop_t colorselector_prop;
		ctxpopup_prop_t ctxpopup_prop;
		datetime_prop_t datetime_prop;
		entry_prop_t entry_prop;
		flip_prop_t flip_prop;
		gengrid_prop_t gengrid_prop;
		genlist_prop_t genlist_prop;
		glview_prop_t glview_prop;
		icon_prop_t icon_prop;
		elm_image_prop_t elm_image_prop;
		index_prop_t index_prop;
		label_prop_t label_prop;
		list_prop_t list_prop;
		map_prop_t map_prop;
		notify_prop_t notify_prop;
		panel_prop_t panel_prop;
		photo_prop_t photo_prop;
		photocam_prop_t photocam_prop;
		popup_prop_t popup_prop;
		progressbar_prop_t progressbar_prop;
		radio_prop_t radio_prop;
		segmencontrol_prop_t segmencontrol_prop;
		slider_prop_t slider_prop;
		spinner_prop_t spinner_prop;
		toolbar_prop_t toolbar_prop;
		tooltip_prop_t tooltip_prop;
		win_prop_t win_prop;
		struct tm currtime;

		case UI_EVAS_IMAGE:
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
			image_prop.frame_count = evas_object_image_animated_frame_count_get(obj);
			image_prop.evas_image_stride = evas_object_image_stride_get(obj);

			to = _pack_image_prop(to, &image_prop);
			break;
		case UI_EVAS_LINE:
			evas_object_line_xy_get(obj, &(line_prop.xy[0]),
						     &(line_prop.xy[1]),
						     &(line_prop.xy[2]),
						     &(line_prop.xy[3]));

			to = _pack_line_prop(to, &line_prop);
			break;
		case UI_EVAS_TEXT:
			evas_object_text_font_get(obj, &(text_prop.font), &(text_prop.size));
			text_prop.text = evas_object_text_text_get(obj);
			text_prop.delim = evas_object_text_bidi_delimiters_get(obj);
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

			to = _pack_text_prop(to, &text_prop);
			break;
		case UI_EVAS_TEXTBLOCK:
			textblock_prop.replace_char = evas_object_textblock_replace_char_get(obj);
			textblock_prop.valign = evas_object_textblock_valign_get(obj);
			textblock_prop.delim = evas_object_textblock_bidi_delimiters_get(obj);
			textblock_prop.newline = evas_object_textblock_legacy_newline_get(obj);
			textblock_prop.markup = evas_object_textblock_text_markup_get(obj);

			to = _pack_textblock_prop(to, &textblock_prop);
			break;
		case UI_EVAS_TABLE:
			table_prop.homogeneous = evas_object_table_homogeneous_get(obj);
			evas_object_table_align_get(obj, &(table_prop.align[0]),
							 &(table_prop.align[1]));
			evas_object_table_padding_get(obj, &(table_prop.padding[0]),
							   &(table_prop.padding[1]));
			table_prop.mirrored = evas_object_table_mirrored_get(obj);
			evas_object_table_col_row_size_get(obj, &(table_prop.col_row_size[0]),
								&(table_prop.col_row_size[1]));

			to = _pack_table_prop(to, &table_prop);
			break;
		case UI_EVAS_BOX:
			evas_object_box_align_get(obj, &(box_prop.align[0]),
						       &(box_prop.align[1]));
			evas_object_box_padding_get(obj, &(box_prop.padding[0]),
							 &(box_prop.padding[1]));

			to = _pack_box_prop(to, &box_prop);
			break;
		case UI_EVAS_GRID:
			grid_prop.mirrored = evas_object_grid_mirrored_get(obj);

			to = _pack_grid_prop(to, &grid_prop);
			break;
		case UI_EVAS_TEXTGRID:
			evas_object_textgrid_size_get(obj, &(textgrid_prop.size[0]),
							   &(textgrid_prop.size[1]));
			evas_object_textgrid_cell_size_get(obj, &(textgrid_prop.cell_size[0]),
								&(textgrid_prop.cell_size[1]));

			to = _pack_textgrid_prop(to, &textgrid_prop);
			break;
		case UI_ELM_BACKGROUND:
			elm_bg_color_get(obj, &(bg_prop.color[0]),
					      &(bg_prop.color[1]),
					      &(bg_prop.color[2]));
			bg_prop.option = elm_bg_option_get(obj);

			to = _pack_bg_prop(to, &bg_prop);
			break;
		case UI_ELM_BUTTON:
			button_prop.initial_timeout = elm_button_autorepeat_initial_timeout_get(obj);
			button_prop.gap_timeout = elm_button_autorepeat_gap_timeout_get(obj);
			button_prop.autorepeat = elm_button_autorepeat_get(obj);

			to = _pack_button_prop(to, &button_prop);
			break;
		case UI_ELM_CHECK:
			check_prop.state = elm_check_state_get(obj);

			to = _pack_check_prop(to, &check_prop);
			break;
		case UI_ELM_COLORSELECTOR:
			elm_colorselector_color_get(obj, &(colorselector_prop.color[0]),
							 &(colorselector_prop.color[1]),
							 &(colorselector_prop.color[2]),
							 &(colorselector_prop.color[3]));
			colorselector_prop.palette_name = elm_colorselector_palette_name_get(obj);
			colorselector_prop.mode = elm_colorselector_mode_get(obj);

			to = _pack_colorselector_prop(to, &colorselector_prop);
			break;
		case UI_ELM_CTXPOPUP:
			ctxpopup_prop.horizontal = elm_ctxpopup_horizontal_get(obj);

			to = _pack_ctxpopup_prop(to, &ctxpopup_prop);
			break;
		case UI_ELM_DATETIME:
			datetime_prop.format = elm_datetime_format_get(obj);
			elm_datetime_value_get(obj, &currtime);
			datetime_prop.value[0] = currtime.tm_sec;
			datetime_prop.value[1] = currtime.tm_min;
			datetime_prop.value[2] = currtime.tm_hour;
			datetime_prop.value[3] = currtime.tm_mday;
			datetime_prop.value[4] = currtime.tm_mon;
			datetime_prop.value[5] = currtime.tm_year;
			datetime_prop.value[6] = currtime.tm_wday;
			datetime_prop.value[7] = currtime.tm_yday;

			to = _pack_datetime_prop(to, &datetime_prop);
			break;
		case UI_ELM_ENTRY:
			entry_prop.entry = elm_entry_entry_get(obj);
			entry_prop.scrollable = elm_entry_scrollable_get(obj);
			entry_prop.panel_show_on_demand = elm_entry_input_panel_show_on_demand_get(obj);
			entry_prop.menu_disabled = elm_entry_context_menu_disabled_get(obj);
			entry_prop.cnp_mode = elm_entry_cnp_mode_get(obj);
			entry_prop.editable = elm_entry_editable_get(obj);
			entry_prop.hover_style = elm_entry_anchor_hover_style_get(obj);
			entry_prop.single_line= elm_entry_single_line_get(obj);
			entry_prop.password = elm_entry_password_get(obj);
			entry_prop.autosave = elm_entry_autosave_get(obj);
			entry_prop.prediction_allow = elm_entry_prediction_allow_get(obj);
			entry_prop.panel_enabled = elm_entry_input_panel_enabled_get(obj);
			entry_prop.cursor_pos = elm_entry_cursor_pos_get(obj);
			entry_prop.cursor_is_format = elm_entry_cursor_is_format_get(obj);
			entry_prop.cursor_content = elm_entry_cursor_content_get(obj);
			entry_prop.selection = elm_entry_selection_get(obj);
			entry_prop.is_visible_format = elm_entry_cursor_is_visible_format_get(obj);

			to = _pack_entry_prop(to, &entry_prop);
			break;
		case UI_ELM_FLIP:
			flip_prop.interaction = elm_flip_interaction_get(obj);
			flip_prop.front_visible = elm_flip_front_visible_get(obj);

			to = _pack_flip_prop(to, &flip_prop);
			break;
		case UI_ELM_GENGRID:
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

			to = _pack_gengrid_prop(to, &gengrid_prop);
			break;
		case UI_ELM_GENLIST:
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
			genlist_prop.realization_mode = elm_genlist_realization_mode_get(obj);

			to = _pack_genlist_prop(to, &genlist_prop);
			break;
		case UI_ELM_GLVIEW:
			elm_glview_size_get(obj, &(glview_prop.size[0]),
						 &(glview_prop.size[1]));
			glview_prop.rotation = elm_glview_rotation_get(obj);

			to = _pack_glview_prop(to, &glview_prop);
			break;
		case UI_ELM_ICON:
			icon_prop.lookup = elm_icon_order_lookup_get(obj);
			icon_prop.standard = elm_icon_standard_get(obj);

			to = _pack_icon_prop(to, &icon_prop);
			break;
		case UI_ELM_IMAGE:
			elm_image_prop.editable = elm_image_editable_get(obj);
			elm_image_prop.play = elm_image_animated_play_get(obj);
			elm_image_prop.smooth = elm_image_smooth_get(obj);
			elm_image_prop.no_scale = elm_image_no_scale_get(obj);
			elm_image_prop.animated = elm_image_animated_get(obj);
			elm_image_prop.aspect_fixed = elm_image_aspect_fixed_get(obj);
			elm_image_prop.orient = elm_image_orient_get(obj);
			elm_image_prop.fill_outside = elm_image_fill_outside_get(obj);
			elm_image_resizable_get(obj, &(elm_image_prop.resizable[0]),
						     &(elm_image_prop.resizable[1]));
			elm_image_prop.animated_available = elm_image_animated_available_get(obj);
			elm_image_object_size_get(obj, &(elm_image_prop.size[0]),
						       &(elm_image_prop.size[1]));

			to = _pack_elm_image_prop(to, &elm_image_prop);
			break;
		case UI_ELM_INDEX:
			index_prop.autohide_disabled = elm_index_autohide_disabled_get(obj);
			index_prop.omit_enabled = elm_index_omit_enabled_get(obj);
			index_prop.priority = elm_index_priority_get(obj);
			index_prop.horizontal = elm_index_horizontal_get(obj);
			index_prop.change_time = elm_index_delay_change_time_get(obj);
			index_prop.indicator_disabled = elm_index_indicator_disabled_get(obj);
			index_prop.item_level = elm_index_item_level_get(obj);

			to = _pack_index_prop(to, &index_prop);
			break;
		case UI_ELM_LABEL:
			label_prop.wrap_width = elm_label_wrap_width_get(obj);
			label_prop.speed = elm_label_slide_speed_get(obj);
			label_prop.mode = elm_label_slide_mode_get(obj);

			to = _pack_label_prop(to, &label_prop);
			break;
		case UI_ELM_LIST:
			list_prop.horizontal = elm_list_horizontal_get(obj);
			list_prop.select_mode = elm_list_select_mode_get(obj);
			list_prop.focus_on_selection = elm_list_focus_on_selection_get(obj);
			list_prop.multi_select = elm_list_multi_select_get(obj);
			list_prop.multi_select_mode = elm_list_multi_select_mode_get(obj);
			list_prop.mode = elm_list_mode_get(obj);

			to = _pack_list_prop(to, &list_prop);
			break;
		case UI_ELM_MAP:
			map_prop.zoom = elm_map_zoom_get(obj);
			map_prop.paused = elm_map_paused_get(obj);
			map_prop.wheel_disabled = elm_map_wheel_disabled_get(obj);
			map_prop.zoom_min = elm_map_zoom_min_get(obj);
			elm_map_rotate_get(obj, &(map_prop.rotate_degree),
						&(map_prop.rotate[0]),
						&(map_prop.rotate[1]));
			map_prop.agent = elm_map_user_agent_get(obj);
			map_prop.zoom_max = elm_map_zoom_max_get(obj);
			map_prop.zoom_mode = elm_map_zoom_mode_get(obj);
			elm_map_region_get(obj, &(map_prop.region[0]),
						&(map_prop.region[1]));

			to = _pack_map_prop(to, &map_prop);
			break;
		case UI_ELM_NOTIFY:
			elm_notify_align_get(obj, &(notify_prop.align[0]),
						  &(notify_prop.align[1]));
			notify_prop.allow_events = elm_notify_allow_events_get(obj);
			notify_prop.timeout = elm_notify_timeout_get(obj);

			to = _pack_notify_prop(to, &notify_prop);
			break;
		case UI_ELM_PANEL:
			panel_prop.orient = elm_panel_orient_get(obj);
			panel_prop.hidden = elm_panel_hidden_get(obj);
			panel_prop.scrollable = elm_panel_scrollable_get(obj);

			to = _pack_panel_prop(to, &panel_prop);
			break;
		case UI_ELM_PHOTO:
			photo_prop.editable = elm_photo_editable_get(obj);
			photo_prop.fill_inside = elm_photo_fill_inside_get(obj);
			photo_prop.aspect_fixed = elm_photo_aspect_fixed_get(obj);
			photo_prop.size = elm_photo_size_get(obj);

			to = _pack_photo_prop(to, &photo_prop);
			break;
		case UI_ELM_PHOTOCAM:
			photocam_prop.paused = elm_photocam_paused_get(obj);
			photocam_prop.file = elm_photocam_file_get(obj);
			photocam_prop.gesture_enabled = elm_photocam_gesture_enabled_get(obj);
			photocam_prop.zoom = elm_photocam_zoom_get(obj);
			photocam_prop.zoom_mode = elm_photocam_zoom_mode_get(obj);
			elm_photocam_image_size_get(obj, &(photocam_prop.image_size[0]),
							 &(photocam_prop.image_size[1]));

			to = _pack_photocam_prop(to, &photocam_prop);
			break;
		case UI_ELM_POPUP:
			elm_popup_align_get(obj, &(popup_prop.align[0]),
						 &(popup_prop.align[1]));
			popup_prop.allow_events = elm_popup_allow_events_get(obj);
			popup_prop.wrap_type = elm_popup_content_text_wrap_type_get(obj);
			popup_prop.orient = elm_popup_orient_get(obj);
			popup_prop.timeout = elm_popup_timeout_get(obj);

			to = _pack_popup_prop(to, &popup_prop);
			break;
		case UI_ELM_PROGRESSBAR:
			progressbar_prop.span_size = elm_progressbar_span_size_get(obj);
			progressbar_prop.pulse = elm_progressbar_pulse_get(obj);
			progressbar_prop.value = elm_progressbar_value_get(obj);
			progressbar_prop.inverted = elm_progressbar_inverted_get(obj);
			progressbar_prop.horizontal = elm_progressbar_horizontal_get(obj);
			progressbar_prop.unit_format = elm_progressbar_unit_format_get(obj);

			to = _pack_progressbar_prop(to, &progressbar_prop);
			break;
		case UI_ELM_RADIO:
			radio_prop.state_value = elm_radio_state_value_get(obj);
			radio_prop.value = elm_radio_value_get(obj);

			to = _pack_radio_prop(to, &radio_prop);
			break;
		case UI_ELM_SEGMENTCONTROL:
			segmencontrol_prop.item_count = elm_segment_control_item_count_get(obj);

			to = _pack_segmencontrol_prop(to, &segmencontrol_prop);
			break;
		case UI_ELM_SLIDER:
			slider_prop.horizontal = elm_slider_horizontal_get(obj);
			slider_prop.value = elm_slider_value_get(obj);
			slider_prop.format = elm_slider_indicator_format_get(obj);

			to = _pack_slider_prop(to, &slider_prop);
			break;
		case UI_ELM_SPINNER:
			elm_spinner_min_max_get(obj, &(spinner_prop.min_max[0]),
						     &(spinner_prop.min_max[1]));
			spinner_prop.step = elm_spinner_step_get(obj);
			spinner_prop.wrap = elm_spinner_wrap_get(obj);
			spinner_prop.interval = elm_spinner_interval_get(obj);
			spinner_prop.round = elm_spinner_round_get(obj);
			spinner_prop.editable = elm_spinner_editable_get(obj);
			spinner_prop.base = elm_spinner_base_get(obj);
			spinner_prop.value = elm_spinner_value_get(obj);
			spinner_prop.format = elm_spinner_label_format_get(obj);

			to = _pack_spinner_prop(to, &spinner_prop);
			break;
		case UI_ELM_TOOLBAR:
			toolbar_prop.reorder_mode = elm_toolbar_reorder_mode_get(obj);
			toolbar_prop.transverse_expanded = elm_toolbar_transverse_expanded_get(obj);
			toolbar_prop.homogeneous = elm_toolbar_homogeneous_get(obj);
			toolbar_prop.align = elm_toolbar_align_get(obj);
			toolbar_prop.select_mode = elm_toolbar_select_mode_get(obj);
			toolbar_prop.icon_size = elm_toolbar_icon_size_get(obj);
			toolbar_prop.horizontal = elm_toolbar_horizontal_get(obj);
			toolbar_prop.standard_priority = elm_toolbar_standard_priority_get(obj);
			toolbar_prop.items_count = elm_toolbar_items_count(obj);

			to = _pack_toolbar_prop(to, &toolbar_prop);
			break;
		case UI_ELM_TOOLTIP:
			tooltip_prop.style = elm_object_tooltip_style_get(obj);
			tooltip_prop.window_mode = elm_object_tooltip_window_mode_get(obj);

			to = _pack_tooltip_prop(to, &tooltip_prop);
			break;
		case UI_ELM_WIN:
			win_prop.iconfield = elm_win_iconified_get(obj);
			win_prop.maximized = elm_win_maximized_get(obj);
			win_prop.modal = elm_win_modal_get(obj);
			win_prop.icon_name = elm_win_icon_name_get(obj);
			win_prop.withdrawn = elm_win_withdrawn_get(obj);
			win_prop.role = elm_win_role_get(obj);
			elm_win_size_step_get(obj, &(win_prop.size_step[0]),
						   &(win_prop.size_step[1]));
			win_prop.highlight_style = elm_win_focus_highlight_style_get(obj);
			win_prop.borderless = elm_win_borderless_get(obj);
			win_prop.highlight_enabled = elm_win_focus_highlight_enabled_get(obj);
			win_prop.title = elm_win_title_get(obj);
			win_prop.alpha = elm_win_alpha_get(obj);
			win_prop.urgent = elm_win_urgent_get(obj);
			win_prop.rotation = elm_win_rotation_get(obj);
			win_prop.sticky = elm_win_sticky_get(obj);
			win_prop.highlight_animate = elm_win_focus_highlight_animate_get(obj);
			win_prop.aspect = elm_win_aspect_get(obj);
			win_prop.indicator_opacity = elm_win_indicator_opacity_get(obj);
			win_prop.demand_attention = elm_win_demand_attention_get(obj);
			win_prop.layer = elm_win_layer_get(obj);
			win_prop.profile = elm_win_profile_get(obj);
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
			to = _pack_win_prop(to, &win_prop);
			break;
		default:
			break;
	}

	return to;
}

static void _render_obj(Evas *evas, Evas_Object *obj, struct timeval *tv)
{
	struct timeval start_tv, finish_tv;

	evas_object_hide(obj);
	evas_render(evas);
	gettimeofday(&start_tv, NULL);
	evas_object_show(obj);
	evas_render(evas);
	gettimeofday(&finish_tv, NULL);

	timersub(&finish_tv, &start_tv, tv);
}

Eina_List *_get_rendering_time(Eina_List *render_list, Evas *evas,
			       Evas_Object *obj, Evas_Object *parent)
{
	ui_obj_render_info_t *render_info;
	Eina_List *children;
	Evas_Object *child;

	if (!_get_redraw(obj))
		return render_list;

	render_info = (ui_obj_render_info_t*)malloc(sizeof(ui_obj_render_info_t));

	if (!render_info) {
		ui_viewer_log("Cannot alloc ui_obj_render_info_t: %d bytes\n",
			 sizeof(ui_obj_render_info_t));
		return render_list;
	}

	render_info->id = obj;
	render_info->parent_id = parent;

	_render_obj(evas, obj, &(render_info->tv));

	render_list = eina_list_append(render_list, render_info);

	children = evas_object_smart_members_get(obj);
	EINA_LIST_FREE(children, child) {
		render_list = _get_rendering_time(render_list, evas, child, obj);
	}
	eina_list_free(children);

	return render_list;
}

Eina_List *ui_viewer_get_rendering_time(Evas_Object *obj,
					enum ErrorCode *err_code)
{
	Eina_List *render_list = NULL;
	Eina_Bool exists = EINA_FALSE;
	Eina_Bool redraw = EINA_FALSE;
	Evas *evas;

	exists = _check_obj_exists(obj);
	if (exists)
		redraw = _get_redraw(obj);

	ui_viewer_log("get rendering time for 0x%lx, exists : %d, redraw : %d\n",
		(unsigned long)obj, exists, redraw);

	if (!exists) {
		*err_code = ERR_UI_OBJ_NOT_FOUND;
		return NULL;

	}

	if (!redraw) {
		*err_code = ERR_UI_OBJ_RENDER_FAILED;
		return NULL;
	}

	evas = evas_object_evas_get(obj);

	render_list = _get_rendering_time(render_list, evas, obj,
					 evas_object_smart_parent_get(obj));

	*err_code = ERR_NO;

	return render_list;
}

static char *_pack_ui_obj_render_info(char *to, ui_obj_render_info_t *info)
{
	ui_viewer_log("pack obj: 0x%lx, parent : 0x%lx, rendering time: %lu sec, %lu usec\n",
		(unsigned long)info->id, (unsigned long)info->parent_id,
		(unsigned long)info->tv.tv_sec, (unsigned long)info->tv.tv_usec);

	to = pack_ptr(to, info->id);
	to = pack_ptr(to, info->parent_id);
	to = pack_timeval(to, info->tv);

	return to;
}

char *pack_ui_obj_render_list(char *to, Eina_List *ui_obj_info_list,
			      enum ErrorCode err_code)
{
	ui_obj_render_info_t *info_i;
	unsigned int info_cnt = eina_list_count(ui_obj_info_list);

	to = pack_int32(to, err_code);

	to = pack_int32(to, info_cnt);

	EINA_LIST_FREE(ui_obj_info_list, info_i)
	{
		to = _pack_ui_obj_render_info(to, info_i);
	}
	eina_list_free(ui_obj_info_list);

	return to;
}
