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

#define MAX_PATH_LENGTH		256
#define MAX_TEXT_LENGTH		1024

enum ErrorCode {
	ERR_NO				= 0,	/* success */
	ERR_ALREADY_RUNNING		= -102,	/* already running */
	ERR_UI_OBJ_NOT_FOUND		= -207,	/* requested ui object is not found */
	ERR_UI_OBJ_SCREENSHOT_FAILED	= -208,	/* requested ui object is in background */
	ERR_UNKNOWN			= -999	/* unknown error */
};

enum hierarchy_status_t {
	HIERARCHY_NOT_RUNNING,
	HIERARCHY_RUNNING,
	HIERARCHY_CANCELLED
};

enum rendering_option_t {
	RENDER_NONE = 0x0,
	RENDER_ALL = 0x1,
	RENDER_ELM = 0x2,
	RENDER_NONE_HOST_OPT = 0x3 /* it's equal to RENDER_NONE for target*/
};

enum ui_obj_category_t {
	UI_UNDEFINED	= 0x00,
	UI_EVAS		= 0x01,
	UI_ELM		= 0x02,
	UI_EDJE		= 0x03
};

enum ui_obj_code_t {
	UI_CODE_UNDEFINED	= 0x0000,
	/* Evas */
	UI_EVAS_UNDEFINED	= 0x0100,
	UI_EVAS_IMAGE		= 0x0101,
	UI_EVAS_LINE		= 0x0102,
	UI_EVAS_POLYGON		= 0X0103,
	UI_EVAS_RECTANGLE	= 0X0104,
	UI_EVAS_TEXT		= 0x0105,
	UI_EVAS_TEXTBLOCK	= 0x0106,
	UI_EVAS_VECTORS		= 0x0107,
	UI_EVAS_TABLE		= 0x0108,
	UI_EVAS_BOX		= 0x0109,
	UI_EVAS_GRID		= 0x010A,
	UI_EVAS_TEXTGRID	= 0x010B,
	UI_EVAS_SMART		= 0x010C,
	/* Elementary */
	UI_ELM_UNDEFINED	= 0x0200,
	UI_ELM_BACKGROUND	= 0x0201,
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
	Evas_Object *id;
	enum ui_obj_category_t category;
	enum ui_obj_code_t code;
	char name[MAX_PATH_LENGTH];
	struct timeval rendering_tv;
	Evas_Object *parent_id;
} ui_obj_info_t;

typedef struct _ui_obj_elm_prop_t {
	char text[MAX_TEXT_LENGTH];
	char style[MAX_PATH_LENGTH];
	Eina_Bool disabled;
	char type[MAX_PATH_LENGTH];
} ui_obj_elm_prop_t;

typedef struct _ui_obj_edje_prop_t {
	Eina_Bool animation;
	Eina_Bool play;
	double scale;
	double base_scale;
	Evas_Coord size_min[2];
	Evas_Coord size_max[2];
} ui_obj_edje_prop_t;

typedef struct _ui_obj_prop_t {
	Evas_Coord geometry[4];
	Eina_Bool focus;
	char name[MAX_PATH_LENGTH];
	Eina_Bool visible;
	int color[4];
	Eina_Bool anti_alias;
	float scale;
	Evas_Coord size_min[2];
	Evas_Coord size_max[2];
	Evas_Coord size_request[2];
	double size_align[2];
	double size_weight[2];
	Evas_Coord size_padding[4];
	Evas_Render_Op render_op;
} ui_obj_prop_t;

typedef struct _image_prop_t {
	double load_dpi;
	Eina_Bool source_clip;
	Eina_Bool filled;
	char content_hint;
	Eina_Bool alpha;
	int border[4];
	double border_scale;
	Eina_Bool pixels_dirty;
	Eina_Bool load_orientation;
	char border_center_fill;
	int size[2];
	Eina_Bool source_visible;
	Evas_Coord fill[4];
	int load_scale_down;
	char scale_hint;
	Eina_Bool source_events;
	int frame_count;
	int evas_image_stride;
} image_prop_t;

typedef struct _line_prop_t {
	Evas_Coord xy[4];
} line_prop_t;

typedef struct _text_prop_t {
	char font[MAX_PATH_LENGTH];
	int size;
	char text[MAX_PATH_LENGTH];
	char delim[MAX_PATH_LENGTH];
	double ellipsis;
	char style;
	int shadow_color[4];
	int glow_color[4];
	int glow2_color[4];
	int outline_color[4];
	int style_pad[4];
	char direction;
} text_prop_t;

typedef struct _textblock_prop_t {
	char replace_char[MAX_PATH_LENGTH];
	double valign;
	char delim[MAX_PATH_LENGTH];
	Eina_Bool newline;
	char markup[MAX_PATH_LENGTH];
} textblock_prop_t;

typedef struct _table_prop_t {
	char homogeneous;
	double align[2];
	Evas_Coord padding[2];
	Eina_Bool mirrored;
	int col_row_size[2];
} table_prop_t;

typedef struct _box_prop_t {
	double align[2];
} box_prop_t;

typedef struct _grid_prop_t {
	Eina_Bool mirrored;
} grid_prop_t;

typedef struct _textgrid_prop_t {
	int size[2];
	int cell_size[2];
} textgrid_prop_t;

typedef struct _bg_prop_t {
	int color[3];
	char option;
} bg_prop_t;

typedef struct _button_prop_t {
	double initial_timeout;
	double gap_timeout;
	Eina_Bool autorepeat;
} button_prop_t;

typedef struct _check_prop_t {
	Eina_Bool state;
} check_prop_t;

typedef struct _colorselector_prop_t {
	int color[4];
	char palette_name[MAX_PATH_LENGTH];
	char mode;
} colorselector_prop_t;

typedef struct _ctxpopup_prop_t {
	Eina_Bool horizontal;
} ctxpopup_prop_t;

typedef struct _datetime_prop_t {
	char format[MAX_PATH_LENGTH];
	int value[8];
} datetime_prop_t;

typedef struct _entry_prop_t {
	char entry[MAX_PATH_LENGTH];
	Eina_Bool scrollable;
	Eina_Bool panel_show_on_demand;
	Eina_Bool menu_disabled;
	char cnp_mode;
	Eina_Bool editable;
	char hover_style[MAX_PATH_LENGTH];
	Eina_Bool single_line;
	Eina_Bool password;
	Eina_Bool autosave;
	Eina_Bool prediction_allow;
	Eina_Bool panel_enabled;
	int cursor_pos;
	Eina_Bool cursor_is_format;
	char *cursor_content;
	char selection[MAX_PATH_LENGTH];
	Eina_Bool is_visible_format;
} entry_prop_t;

typedef struct _flip_prop_t {
	char interaction;
	Eina_Bool front_visible;
} flip_prop_t;

typedef struct _gengrid_prop_t {
	double align[2];
	Eina_Bool filled;
	double relative[2];
	Eina_Bool multi_select;
	Evas_Coord group_item_size[2];
	char select_mode;
	Eina_Bool render_mode;
	Eina_Bool highlight_mode;
	Evas_Coord item_size[2];
	char multi_select_mode;
	Eina_Bool horizontal;
	Eina_Bool wheel_disabled;
	int items_count;
} gengrid_prop_t;

typedef struct _genlist_prop_t {
	Eina_Bool multi_select;
	char genlist_mode;
	int items_count;
	Eina_Bool homogeneous;
	int block_count;
	double timeout;
	Eina_Bool reorder_mode;
	Eina_Bool decorate_mode;
	Eina_Bool effect_enabled;
	char select_mode;
	Eina_Bool highlight_mode;
	Eina_Bool realization_mode;
} genlist_prop_t;

typedef struct _glview_prop_t {
	int size[2];
	int rotation;
} glview_prop_t;

typedef struct _icon_prop_t {
	char lookup;
	char standard[MAX_PATH_LENGTH];
} icon_prop_t;

typedef struct _elm_image_prop_t {
	Eina_Bool editable;
	Eina_Bool play;
	Eina_Bool smooth;
	Eina_Bool no_scale;
	Eina_Bool animated;
	Eina_Bool aspect_fixed;
	char orient;
	Eina_Bool fill_outside;
	Eina_Bool resizable[2];
	Eina_Bool animated_available;
	int size[2];
} elm_image_prop_t;

typedef struct _index_prop_t {
	Eina_Bool autohide_disabled;
	Eina_Bool omit_enabled;
	int priority;
	Eina_Bool horizontal;
	double change_time;
	Eina_Bool indicator_disabled;
	int item_level;
} index_prop_t;

typedef struct _label_prop_t {
	int wrap_width;
	double speed;
	char mode;
} label_prop_t;

typedef struct _list_prop_t {
	Eina_Bool horizontal;
	char select_mode;
	Eina_Bool focus_on_selection;
	Eina_Bool multi_select;
	char multi_select_mode;
	char mode;
} list_prop_t;

typedef struct _map_prop_t {
	int zoom;
	Eina_Bool paused;
	Eina_Bool wheel_disabled;
	int zoom_min;
	double rotate_degree;
	Evas_Coord rotate[2];
	char agent[MAX_PATH_LENGTH];
	int zoom_max;
	char zoom_mode;
	double region[2];
} map_prop_t;

typedef struct _notify_prop_t {
	double align[2];
	Eina_Bool allow_events;
	double timeout;
} notify_prop_t;

typedef struct _panel_prop_t {
	char orient;
	Eina_Bool hidden;
	Eina_Bool scrollable;
} panel_prop_t;

typedef struct _photo_prop_t {
	Eina_Bool editable;
	Eina_Bool fill_inside;
	Eina_Bool aspect_fixed;
	int size;
} photo_prop_t;

typedef struct _photocam_prop_t {
	Eina_Bool paused;
	char file[MAX_PATH_LENGTH];
	Eina_Bool gesture_enabled;
	double zoom;
	char zoom_mode;
	int image_size[2];
} photocam_prop_t;

typedef struct _popup_prop_t {
	double align[2];
	Eina_Bool allow_events;
	char wrap_type;
	char orient;
	double timeout;
} popup_prop_t;

typedef struct _progressbar_prop_t {
	Evas_Coord span_size;
	Eina_Bool pulse;
	double value;
	Eina_Bool inverted;
	Eina_Bool horizontal;
	char unit_format[MAX_PATH_LENGTH];
} progressbar_prop_t;

typedef struct _radio_prop_t {
	int state_value;
	int value;
} radio_prop_t;

typedef struct _segmencontrol_prop_t {
	int item_count;
} segmencontrol_prop_t;

typedef struct _slider_prop_t {
	Eina_Bool horizontal;
	double value;
	char format[MAX_PATH_LENGTH];
} slider_prop_t;

typedef struct _spinner_prop_t {
	double min_max[2];
	double step;
	Eina_Bool wrap;
	double interval;
	int round;
	Eina_Bool editable;
	double base;
	double value;
	char format[MAX_PATH_LENGTH];
} spinner_prop_t;

typedef struct _toolbar_prop_t {
	Eina_Bool reorder_mode;
	Eina_Bool transverse_expanded;
	Eina_Bool homogeneous;
	double align;
	char select_mode;
	int icon_size;
	Eina_Bool horizontal;
	int standard_priority;
	int items_count;
} toolbar_prop_t;

typedef struct _tooltip_prop_t {
	char style[MAX_PATH_LENGTH];
	Eina_Bool window_mode;
} tooltip_prop_t;

typedef struct _win_prop_t {
	Eina_Bool iconfield;
	Eina_Bool maximized;
	Eina_Bool modal;
	char icon_name[MAX_PATH_LENGTH];
	Eina_Bool withdrawn;
	char role[MAX_PATH_LENGTH];
	int size_step[2];
	char highlight_style[MAX_PATH_LENGTH];
	Eina_Bool borderless;
	Eina_Bool highlight_enabled;
	char title[MAX_PATH_LENGTH];
	Eina_Bool alpha;
	Eina_Bool urgent;
	int rotation;
	Eina_Bool sticky;
	Eina_Bool highlight_animate;
	double aspect;
	char indicator_opacity;
	Eina_Bool demand_attention;
	int layer;
	char profile[MAX_PATH_LENGTH];
	Eina_Bool shaped;
	Eina_Bool fullscreen;
	char indicator_mode;
	Eina_Bool conformant;
	int size_base[2];
	Eina_Bool quickpanel;
	Eina_Bool rotation_supported;
	int screen_dpi[2];
	char win_type;
} win_prop_t;

char *pack_string(char *to, const char *str);
void pack_ui_obj_info_list(FILE *file, enum rendering_option_t rendering,
			    Eina_Bool *cancelled);
char *pack_ui_obj_screenshot(char *to, Evas_Object *obj);
enum hierarchy_status_t get_hierarchy_status(void);
void set_hierarchy_status(enum hierarchy_status_t status);

static inline char *pack_int32(char *to, uint32_t val)
{
	*(uint32_t *)to = val;
	return to + sizeof(uint32_t);
}

static inline void write_int8(FILE *file, uint8_t val)
{
	fwrite(&val, sizeof(val), 1, file);
}

static inline void write_int32(FILE *file, uint32_t val)
{
	fwrite(&val, sizeof(val), 1, file);
}

static inline void write_int64(FILE *file, uint64_t val)
{
	fwrite(&val, sizeof(val), 1, file);
}

static inline void write_float(FILE *file, float val)
{
	fwrite(&val, sizeof(val), 1, file);
}
static inline void write_ptr(FILE *file, const void *val)
{
	uint64_t ptr = (uint64_t)(uintptr_t)val;

	fwrite(&ptr, sizeof(ptr), 1, file);
}

static inline void write_timeval(FILE *file, struct timeval tv)
{
	write_int32(file, tv.tv_sec);
	write_int32(file, tv.tv_usec * 1000);
}

static inline void write_string(FILE  *file, const char *str)
{
	size_t len = strlen(str) + 1;

	fwrite(str, len, 1, file);
}

#endif /* _UI_VIEWER_DATA_ */
