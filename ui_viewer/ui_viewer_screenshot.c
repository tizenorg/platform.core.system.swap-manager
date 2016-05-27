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
#include "wayland-api.h"

#include "ui_viewer_utils.h"

#define MAX_HEIGHT		720
#define CAPTURE_TIMEOUT		2.0
#define MAX_PATH_LENGTH		256

static int screenshotIndex = 0;
static pthread_mutex_t captureScreenLock = PTHREAD_MUTEX_INITIALIZER;

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
	struct screenshot_data *sdata = data;
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
	struct screenshot_data *sdata = data;

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
	} else if (strcmp(interface, "wl_shm") == 0) {
		sdata->shm = wl_registry_bind(registry, name,
					      &wl_shm_interface, 1);
	} else if (strcmp(interface, "screenshooter") == 0) {
		sdata->screenshooter = wl_registry_bind(registry, name,
						 &screenshooter_interface,
						 1);
	}
}

static const struct wl_registry_listener registry_listener = {
	handle_global,
	NULL
};

static struct wl_buffer *create_shm_buffer(struct wl_shm *shm, int width,
					   int height, void **data_out)
{
	char filename[] = "/tmp/wayland-shm-XXXXXX";
	struct wl_shm_pool *pool;
	struct wl_buffer *buffer;
	int fd, size, stride;
	void *data;

	stride = width * 4;
	size = stride * height;

	fd = mkstemp(filename);
	if (fd < 0)
		return NULL;

	if (ftruncate(fd, size) < 0) {
		close(fd);
		return NULL;
	}

	data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	unlink(filename);

	if (data == MAP_FAILED) {
		close(fd);
		return NULL;
	}

	pool = wl_shm_create_pool(shm, fd, size);
	close(fd);
	buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
					   WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);
	*data_out = data;

	return buffer;
}

static void *__screenshot_to_buf(struct screenshot_data *sdata, int x, int y,
				 int width, int height)
{
	int dest_stride, src_stride, i;
	void *data, *dest, *src;
	struct screenshooter_output *output, *next;
	dest_stride = width * 4;

	data = malloc(width * height * 4);
	if (!data)
		return NULL;

	wl_list_for_each_safe(output, next, &sdata->output_list, link) {
		src_stride = output->width * 4;
		dest = data;
		if (y + height <= output->height) {
			src = output->data + y * src_stride + x * 4;

			for (i = 0; i < height; i++) {
				memcpy(dest, src, dest_stride);
				dest += dest_stride;
				src += src_stride;
			}
		} else {
			PRINTERR("Cannot get screenshot. wrong height."
				 "src_height=%d [y=%d, h=%d]", output->height,
				 y, height);
			memset(dest, 0xab, height * width * 4);
		}

		wl_list_remove(&output->link);
		free(output);
	}

	return data;
}

static int
set_buffer_size(struct screenshot_data *sdata, int *width, int *height)
{
	struct screenshooter_output *output;

	sdata->min_x = sdata->min_y = INT_MAX;
	sdata->max_x = sdata->max_y = INT_MIN;
	int position = 0;

	wl_list_for_each_reverse(output, &sdata->output_list, link) {
		output->offset_x = position;
		position += output->width;
	}

	wl_list_for_each(output, &sdata->output_list, link) {
		sdata->min_x = MIN(sdata->min_x, output->offset_x);
		sdata->min_y = MIN(sdata->min_y, output->offset_y);
		sdata->max_x = MAX(sdata->max_x,
				   output->offset_x + output->width);
		sdata->max_y = MAX(sdata->max_y,
				   output->offset_y + output->height);
	}

	if (sdata->max_x <= sdata->min_x || sdata->max_y <= sdata->min_y)
		return -1;

	*width = sdata->max_x - sdata->min_x;
	*height = sdata->max_y - sdata->min_y;

	return 0;
}

static void *__capture_screnshot_wayland(int x, int y, int width, int height)
{
	struct wl_display *display = NULL;
	struct wl_registry *registry;
	struct screenshooter_output *output;
	void *buf = NULL;
	struct screenshot_data *sdata;
	const char *wayland_socket = NULL;

	wayland_socket = getenv("WAYLAND_SOCKET");
	if (!wayland_socket)
		wayland_socket = getenv("WAYLAND_DISPLAY");

	if (!wayland_socket) {
		PRINTERR("must be launched by wayland");
		return NULL;
	}

	sdata = malloc(sizeof(*sdata));
	if (!sdata)
		return NULL;

	sdata->shm = NULL;
	sdata->screenshooter = NULL;
	wl_list_init(&sdata->output_list);
	sdata->min_x = 0;
	sdata->min_y = 0;
	sdata->max_x = 0;
	sdata->max_y = 0;
	sdata->buffer_copy_done = 0;

	display = wl_display_connect(wayland_socket);
	if (display == NULL)
		goto out;

	/* wl_list_init(&output_list); */
	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, sdata);
	wl_display_dispatch(display);
	wl_display_roundtrip(display);

	if (sdata->screenshooter == NULL) {
		PRINTERR("display doesn't support screenshooter");
		return NULL;
	}

	screenshooter_add_listener(sdata->screenshooter,
				   &screenshooter_listener, sdata);

	int buf_width, buf_height;
	if (set_buffer_size(sdata, &buf_width, &buf_height))
		return NULL;

	wl_list_for_each(output, &sdata->output_list, link) {
		output->buffer = create_shm_buffer(sdata->shm, output->width,
						   output->height,
						   &output->data);
		screenshooter_shoot(sdata->screenshooter, output->output,
				    output->buffer);
		sdata->buffer_copy_done = 0;
		while (!sdata->buffer_copy_done) {
			wl_display_roundtrip(display);
		}
	}

	buf = __screenshot_to_buf(sdata, x, y, width, height);

out:
	if (display)
		wl_display_disconnect(display);
	free(sdata);
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
		strcpy(screenshot_path, dstpath);
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
