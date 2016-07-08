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

#include <stdlib.h>		// for system
#include <sys/types.h>		// for stat, getpid
#include <sys/stat.h>		// fot stat, chmod
#include <unistd.h>		// fot stat, getpid
#include <pthread.h>		// for mutex
#include <sys/param.h>		// MIN, MAX
#include <sys/mman.h>		// mmap

#include <Ecore.h>
#include <Evas.h>
#include <Evas_Engine_Buffer.h>
#include <Elementary.h>

#include <wayland-client.h>
#include <wayland-tbm-client.h>
#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#include <tizen-extension-client-protocol.h>
#include <screenshooter-client-protocol.h>

#include "ui_viewer_utils.h"

#define MAX_PATH_LENGTH		256

static int screenshotIndex = 0;
static pthread_mutex_t captureScreenLock = PTHREAD_MUTEX_INITIALIZER;

struct efl_data {
	struct wl_display *wl_display;
	struct wl_registry *wl_registry;
	struct screenshooter *screenshooter;
	struct wl_list output_list;
	int min_x, min_y, max_x, max_y;
	int buffer_copy_done;
	int width, height;
};

struct screenshot_data {
	struct wl_shm *shm;
	struct screenshooter *screenshooter;
	struct wl_list output_list;
	int min_x, min_y, max_x, max_y;
	int buffer_copy_done;
};

struct screenshooter_output {
	struct wl_output *output;
	struct wl_buffer *buffer;
	int width, height, offset_x, offset_y;
	void *data;
	struct wl_list link;
};

static void
display_handle_geometry(void __attribute__((unused)) *data,
			struct wl_output *wl_output,
			int x,
			int y,
			int __attribute__((unused)) physical_width,
			int __attribute__((unused)) physical_height,
			int __attribute__((unused)) subpixel,
			const char __attribute__((unused)) *make,
			const char __attribute__((unused)) *model,
			int __attribute__((unused)) transform)
{
	struct screenshooter_output *output;

	output = wl_output_get_user_data(wl_output);

	if (wl_output == output->output) {
		output->offset_x = x;
		output->offset_y = y;
	}
}

static void
display_handle_mode(void __attribute__((unused)) *data,
		    struct wl_output *wl_output, uint32_t flags, int width,
		    int height, int __attribute__((unused)) refresh)
{
	struct screenshooter_output *output;

	output = wl_output_get_user_data(wl_output);

	if ((wl_output == output->output) && (flags & WL_OUTPUT_MODE_CURRENT)) {
		output->width = width;
		output->height = height;
	}
}

static const struct wl_output_listener output_listener = {
	display_handle_geometry,
	display_handle_mode,
	NULL,
	NULL
};

static void
screenshot_done(void *data,
		struct screenshooter __attribute__((unused)) *screenshooter)
{
	struct efl_data *sdata = data;
	sdata->buffer_copy_done = 1;
}

static const struct screenshooter_listener screenshooter_listener = {
	screenshot_done
};

static void
handle_global(void *data, struct wl_registry *registry,
	      uint32_t name, const char *interface,
	      uint32_t __attribute__((unused)) version)
{
	struct efl_data *sdata = data;

	if (strcmp(interface, "wl_output") == 0) {
		struct screenshooter_output *output = malloc(sizeof(*output));

		if (output) {
			PRINTMSG("allocate %p", output);
			output->output = wl_registry_bind(registry, name,
							  &wl_output_interface,
							  1);
			wl_list_insert(&sdata->output_list, &output->link);
			wl_output_add_listener(output->output,
					       &output_listener, output);
		}
	} else if (strcmp(interface, "screenshooter") == 0) {
		sdata->screenshooter = wl_registry_bind(registry, name,
						 &screenshooter_interface,
						 1);
		screenshooter_add_listener(sdata->screenshooter,
					   &screenshooter_listener,
					   sdata);
	}
}

static const struct wl_registry_listener registry_listener = {
	handle_global,
	NULL
};

static struct efl_data *__edata = NULL;

void wayland_deinit(void)
{
	struct screenshooter_output *output, *next;

	if (!__edata)
		return;

	wl_list_for_each_safe(output, next, &__edata->output_list, link)
		free(output);

	wl_registry_destroy(__edata->wl_registry);
	wl_display_disconnect(__edata->wl_display);
	free(__edata);
}

static struct efl_data *__wayland_init(void)
{
	struct wl_display *display = NULL;
	struct wl_registry *registry;
	struct efl_data *data;
	const char *wayland_socket = NULL;

	if (__edata)
		return __edata;

	wayland_socket = getenv("WAYLAND_SOCKET");
	if (!wayland_socket)
		wayland_socket = getenv("WAYLAND_DISPLAY");

	if (!wayland_socket) {
		PRINTERR("must be launched by wayland");
		return NULL;
	}

	data = malloc(sizeof(*data));
	if (!data)
		return NULL;

	data->screenshooter = NULL;
	wl_list_init(&data->output_list);
	data->min_x = 0;
	data->min_y = 0;
	data->max_x = 0;
	data->max_y = 0;
	data->buffer_copy_done = 0;

	display = wl_display_connect(wayland_socket);
	if (display == NULL) {
		fprintf(stderr, "failed to create display: %m\n");
		goto fail;
	}
	data->wl_display = display;

	/* wl_list_init(&output_list); */
	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, data);
	wl_display_dispatch(display);
	wl_display_roundtrip(display);

	data->wl_registry = registry;

	__edata = data;

	return data;
fail:
	free(data);
	return NULL;
}

static void  __set_buffer_size(struct efl_data *data)
{
	struct screenshooter_output *output;

	data->min_x = data->min_y = INT_MAX;
	data->max_x = data->max_y = INT_MIN;
	int position = 0;

	wl_list_for_each_reverse(output, &data->output_list, link) {
		output->offset_x = position;
		position += output->width;
	}

	wl_list_for_each(output, &data->output_list, link) {
		data->min_x = MIN(data->min_x, output->offset_x);
		data->min_y = MIN(data->min_y, output->offset_y);
		data->max_x = MAX(data->max_x, output->offset_x + output->width);
		data->max_y = MAX(data->max_y, output->offset_y + output->height);
	}

	if (data->max_x <= data->min_x || data->max_y <= data->min_y) {
		data->width = 0;
		data->height = 0;
	} else {
		data->width = data->max_x - data->min_x;
		data->height = data->max_y - data->min_y;
	}
}

static void *__tbm_surface_to_buf(unsigned char *src, uint32_t src_stride,
				  uint32_t src_size, int x, int y,
				  int width, int height)
{
	int dest_stride;
	void *data, *dest;
	uint32_t dest_size, n;

	dest_stride = width * 4;
	dest_size = dest_stride * height;

	if (dest_size > src_size)
		return NULL;

	data = malloc(dest_size);
	if (!data)
		return NULL;

	dest = data;
	src = src + y * src_stride + x * 4;

	n = 0;
	while (n < dest_size) {
		memcpy(dest, src, dest_stride);
		n += dest_stride;
	        dest += dest_stride;
		src += src_stride;
	}

	return data;
}

static void *__capture_screnshot_wayland(int x, int y, int width, int height)
{
	struct screenshooter_output *output;
	struct efl_data *data;
	tbm_surface_h t_surface;
	struct wl_buffer *buffer;
	struct wayland_tbm_client *tbm_client;
	tbm_surface_info_s tsuri;
	int plane_idx = 0;
	int err;
	void *buf = NULL;

	data =  __wayland_init();
	if (!data) {
		PRINTERR("failed to init wayland protocol\n");
		return NULL;
	}

	if (!data->screenshooter) {
		PRINTERR("display doesn't support screenshooter\n");
		return NULL;
	}

	__set_buffer_size(data);

	tbm_client = wayland_tbm_client_init(data->wl_display);
	if (!tbm_client) {
		PRINTERR("failed to init tbm client\n");
		return NULL;
	}

	t_surface = tbm_surface_create(data->width, data->height, TBM_FORMAT_XRGB8888);
	if (!t_surface) {
		PRINTERR("failed to create tbm_surface\n");
		goto fail_tbm_client;
	}

	buffer = wayland_tbm_client_create_buffer(tbm_client, t_surface);
	if (!buffer) {
		PRINTERR("failed to create wl_buffer for screenshot\n");
		goto fail_tbm_surface;
	}

	wl_list_for_each(output, &data->output_list, link) {
		screenshooter_shoot(data->screenshooter,
					   output->output,
					   buffer);
		data->buffer_copy_done = 0;
		while (!data->buffer_copy_done) {
			wl_display_roundtrip(data->wl_display);
		}
	}

	err = tbm_surface_map(t_surface,
			      TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE,
			      &tsuri);
	if (err != TBM_SURFACE_ERROR_NONE) {
		PRINTERR("failed to map tbm_surface\n");
		goto fail_map_surface;
	}

        buf = __tbm_surface_to_buf(tsuri.planes[plane_idx].ptr,
				   tsuri.planes[plane_idx].stride,
				   tsuri.planes[plane_idx].size,
				   x, y, width, height);

	tbm_surface_unmap(t_surface);

fail_map_surface:
	wayland_tbm_client_destroy_buffer(tbm_client, buffer);

fail_tbm_surface:
	tbm_surface_destroy(t_surface);

fail_tbm_client:
	wayland_tbm_client_deinit(tbm_client);

	return buf;
}
static int capture_object(char *screenshot_path, int width, int height,
			  Evas_Object *obj, const char *type_name)
{
	char dstpath[MAX_PATH_LENGTH];
	Evas_Object *img;
	Evas *canvas, *sub_canvas;
	Ecore_Evas *ee, *sub_ee;
	int ret = 0;
	char *image_data = NULL;

	screenshotIndex++;

	canvas = evas_object_evas_get(obj);
	ee = ecore_evas_ecore_evas_get(canvas);
	img = ecore_evas_object_image_new(ee);
	evas_object_image_filled_set(img, EINA_TRUE);
	evas_object_image_size_set(img, width, height);
	sub_ee = ecore_evas_object_ecore_evas_get(img);
	sub_canvas = ecore_evas_object_evas_get(img);
	evas_object_resize(img, width, height);
	ecore_evas_resize(sub_ee, width, height);

	if (!strcmp(type_name, "rectangle")) {
		Evas_Object *rect;
		int r, g, b, a;

		rect = evas_object_rectangle_add(sub_canvas);
		evas_object_color_get(obj, &r, &g, &b, &a);
		evas_object_color_set(rect, r, g, b, a);
		evas_object_resize(rect, width, height);
		evas_object_show(rect);
	} else if (!strcmp(type_name, "image")) {
		Evas_Object *image;
		void *img_data;
		int w, h;

		img_data = evas_object_image_data_get(obj, EINA_FALSE);
		evas_object_image_size_get(obj, &w, &h);
		image = evas_object_image_filled_add(sub_canvas);
		evas_object_image_size_set(image, w, h);
		evas_object_image_data_set(image, img_data);
		evas_object_image_data_update_add(image, 0, 0, w, h);
		evas_object_resize(image, width, height);

		evas_object_show(image);
	} else {
		int x, y;
		Evas_Object *image;

		evas_object_geometry_get(obj, &x, &y, NULL, NULL);
		image_data = __capture_screnshot_wayland(x, y, width, height);
		if (image_data == NULL)
			goto finish;
		image = evas_object_image_filled_add(sub_canvas);
		evas_object_image_size_set(image, width, height);
		evas_object_image_data_set(image, image_data);
		evas_object_image_data_update_add(image, 0, 0, width, height);
		evas_object_resize(image, width, height);
		evas_object_show(image);
	}

finish:
	ecore_evas_manual_render(sub_ee);

	snprintf(dstpath, sizeof(dstpath), TMP_DIR "/%d_%d.png", _getpid(),
		 screenshotIndex);

	if (evas_object_image_save(img, dstpath, NULL, "compress=5") != 0) {
		strncpy(screenshot_path, dstpath, MAX_PATH_LENGTH);
	} else {
		ui_viewer_log("ERROR: capture_object : can't save image\n");
		ret = -1;
	}

	evas_object_del(img);
	free(image_data);

	return ret;
}

int ui_viewer_capture_screen(char *screenshot_path, Evas_Object *obj)
{
	int view_w, view_h;
	int obj_x, obj_y, obj_w, obj_h;
	int ret = 0;
	Evas *evas;
	char type_name[MAX_PATH_LENGTH];

	if (!screenshot_path) {
		ui_viewer_log("ui_viewer_capture_screen: no screenshot path\n");
		ret = -1;
		return ret;
	}

	pthread_mutex_lock(&captureScreenLock);

	_strncpy(type_name, evas_object_type_get(obj), MAX_PATH_LENGTH);

	evas = evas_object_evas_get(obj);
	evas_output_viewport_get(evas, NULL, NULL, &view_w, &view_h);
	evas_object_geometry_get(obj, &obj_x, &obj_y, &obj_w, &obj_h);

	if (obj_w == 0 || obj_h == 0) {
		ui_viewer_log("ui_viewer_capture_screen : object %p[%d,%d]"
			      "has zero width or height\n", obj, obj_w, obj_h);
		ret = -1;
	} else if (!strcmp(type_name, "rectangle")) {
		int width, height;

		// restrict rectangle area
		width = (obj_w <= view_w) ? obj_w : view_w;
		height = (obj_h <= view_h) ? obj_h : view_h;

		ret = capture_object(screenshot_path, width, height, obj,
				     type_name);
	} else if (!strcmp(type_name, "image")) {
		ret = capture_object(screenshot_path, obj_w, obj_h, obj,
				     type_name);
	} else if (!strcmp(type_name, "vectors")) {
		ret = -1;
	} else if (!strcmp(type_name, "elm_image") ||
		   !strcmp(type_name, "elm_icon")) {
		Evas_Object *internal_img;
		int img_w, img_h;

		internal_img = elm_image_object_get(obj);
		evas_object_geometry_get(internal_img, NULL, NULL, &img_w,
					 &img_h);

		ret = capture_object(screenshot_path, img_w, img_h,
				     internal_img, type_name);
	} else if (obj_x > view_w || obj_y > view_h) {
		ui_viewer_log("ui_viewer_capture_screen:"
			      "object %p lies beside view area\n", obj);
		ret = -1;
	} else if (!evas_object_visible_get(obj)) {
		ui_viewer_log("ui_viewer_capture_screen:"
			      "object %p is unvisible\n", obj);
		ret = -1;
	} else {
		int width, height;

		// take visible on screen part of object
		width = (view_w < obj_w + obj_x) ? (view_w - obj_x) : obj_w;
		height = (view_h < obj_h + obj_y) ? (view_h - obj_y) : obj_h;

		ret = capture_object(screenshot_path, width, height, obj,
				     type_name);
	}

	if (ret)
		screenshot_path[0] = '\0';

	pthread_mutex_unlock(&captureScreenLock);
	return ret;
}
