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

#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>
#include <dlfcn.h>
#include <poll.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <Ecore.h>
#include <Evas.h>
#include <Evas_Engine_Buffer.h>
#include <Elementary.h>

#include "ui_viewer_utils.h"

typedef struct _screenshot_data
{
	XImage*			ximage;
	Display*		dpy;
	XShmSegmentInfo		x_shm_info;
} screenshot_data;

static int screenshotIndex = 0;

static char* captureScreenShotX(int width, int height, screenshot_data* sdata)
{
	Window root;

	sdata->dpy = XOpenDisplay(NULL);
	if(sdata->dpy == NULL)
	{
		ui_viewer_log("ERROR: XOpenDisplay failed\n");
		return NULL;
	}

	root = RootWindow(sdata->dpy, DefaultScreen(sdata->dpy));

	sdata->ximage = XShmCreateImage(sdata->dpy, DefaultVisualOfScreen (DefaultScreenOfDisplay (sdata->dpy)), 24,
					ZPixmap, NULL, &sdata->x_shm_info, (unsigned int)width, (unsigned int)height);

	if (sdata->ximage != NULL)
	{
		sdata->x_shm_info.shmid = shmget(IPC_PRIVATE, sdata->ximage->bytes_per_line * sdata->ximage->height, IPC_CREAT | 0777);
		sdata->x_shm_info.shmaddr = sdata->ximage->data = shmat(sdata->x_shm_info.shmid, 0, 0);
		sdata->x_shm_info.readOnly = False;

		if (XShmAttach(sdata->dpy, &sdata->x_shm_info))
		{
			if (XShmGetImage(sdata->dpy, root, sdata->ximage, 0, 0, AllPlanes))
			{
				XSync(sdata->dpy, False);
				return sdata->ximage->data;
			}
			else
			{
				ui_viewer_log("ERROR: XShmGetImage failed\n");
			}

			XShmDetach(sdata->dpy, &sdata->x_shm_info);
		}
		else
		{
			ui_viewer_log("ERROR: XShmAttach failed\n");
		}

		shmdt (sdata->x_shm_info.shmaddr);
		shmctl (sdata->x_shm_info.shmid, IPC_RMID, NULL);
		XDestroyImage(sdata->ximage);
		sdata->ximage = NULL;
	}
	else
	{
		ui_viewer_log("ERROR: XShmCreateImage failed\n");
	}

	return NULL;
}

static void releaseScreenShotX(screenshot_data* sdata)
{
	if(sdata->ximage)
	{
		XShmDetach(sdata->dpy, &sdata->x_shm_info);
		shmdt (sdata->x_shm_info.shmaddr);
		shmctl (sdata->x_shm_info.shmid, IPC_RMID, NULL);
		XDestroyImage(sdata->ximage);
	}
	else { }

	if(sdata->dpy)
	{
		XCloseDisplay(sdata->dpy);
	}
}

static Evas* create_canvas(int width, int height)
{
	Evas *canvas;
	Evas_Engine_Info_Buffer *einfo;
	int method;
	void *pixels;

	method = evas_render_method_lookup("buffer");
	if (method <= 0)
	{
		ui_viewer_log("ERROR: evas was not compiled with 'buffer' engine!\n");
		return NULL;
	}

	canvas = evas_new();
	if (canvas == NULL)
	{
		ui_viewer_log("ERROR: could not instantiate new evas canvas.\n");
		return NULL;
	}

	evas_output_method_set(canvas, method);
	evas_output_size_set(canvas, width, height);
	evas_output_viewport_set(canvas, 0, 0, width, height);

	einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(canvas);
	if (einfo == NULL)
	{
		ui_viewer_log("ERROR: could not get evas engine info!\n");
		evas_free(canvas);
		return NULL;
	}

	// ARGB32 is sizeof(int), that is 4 bytes, per pixel
	pixels = malloc(width * height * sizeof(int));
	if (pixels == NULL) {
		ui_viewer_log("ERROR: could not allocate canvas pixels!\n");
		evas_free(canvas);
		return NULL;
	}

	einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
	einfo->info.dest_buffer = pixels;
	einfo->info.dest_buffer_row_bytes = width * sizeof(int);
	einfo->info.use_color_key = 0;
	einfo->info.alpha_threshold = 0;
	einfo->info.func.new_update_region = NULL;
	einfo->info.func.free_update_region = NULL;

	if (evas_engine_info_set(canvas,(Evas_Engine_Info*)einfo) == EINA_FALSE) {
		PRINTMSG("ERROR: could not set evas engine info!\n");
		evas_free(canvas);
		return NULL;
	}

	return canvas;
}

static void destroy_canvas(Evas* canvas)
{
	Evas_Engine_Info_Buffer *einfo;

	einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(canvas);
	if (einfo == NULL)
	{
		ui_viewer_log("ERROR: could not get evas engine info!\n");
		evas_free(canvas);
		return;
	}

	free(einfo->info.dest_buffer);
	evas_free(canvas);
}

static int captureScreen(char *screenshot_path, int width, int height) {
	char dstpath[MAX_PATH_LENGTH];
	char* scrimage;
	screenshot_data sdata;
	Evas_Object* img;
	Evas* ev = NULL;
	int ret = 0;

	screenshotIndex++;

	sdata.ximage = NULL;
	scrimage = captureScreenShotX(width, height, &sdata);
	if(scrimage != NULL)
	{
		ev = create_canvas(width, height);
		if(ev != NULL)
		{
			snprintf(dstpath, sizeof(dstpath),
				 TMP_DIR"/%d_%d.png", _getpid(),
				 screenshotIndex);

			// make image buffer
			if((img = evas_object_image_add(ev)) != NULL)
			{
				//image buffer set
				evas_object_image_data_set(img, NULL);
				evas_object_image_size_set(img, width, height);
				evas_object_image_data_set(img, scrimage);
				evas_object_image_data_update_add(img, 0, 0, width, height);

				//save file
				if(evas_object_image_save(img, dstpath, NULL, "compress=5") != 0)
				{
					if (screenshot_path != NULL) {
						strcpy(screenshot_path, dstpath);
					} else {
						ui_viewer_log("ERROR: captureScreen : wrong screenshot_path\n");
						ret = -1;
					}
				}
				else
				{
					ui_viewer_log("ERROR: captureScreen : evas_object_image_save failed\n");
					ret = -1;
				}
			}
			else
			{
				ui_viewer_log("ERROR: captureScreen : evas_object_image_add failed\n");
				ret = -1;
			}
		}
		else
		{
			ui_viewer_log("ERROR: captureScreen : create canvas failed\n");
			ret = -1;
		}
	}
	else
	{
		ui_viewer_log("ERROR: captureScreen : captureScreenShotX failed\n");
		ret = -1;
	}

	// release resources
	releaseScreenShotX(&sdata);
	if(ev)
		destroy_canvas(ev);

	return ret;
}

static int capture_object(char *screenshot_path, int width, int height, Evas_Object *obj) {
	char dstpath[MAX_PATH_LENGTH];
	Evas_Object *img;
	Evas *canvas, *sub_canvas;
	Ecore_Evas *ee, *sub_ee;
	const char *type_name;
	int ret = 0;

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

	type_name = evas_object_type_get(obj);

	if (!strcmp(type_name, "rectangle")) {
		Evas_Object* rect;
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
	}

	ecore_evas_manual_render(sub_ee);

	snprintf(dstpath, sizeof(dstpath), TMP_DIR"/%d_%d.png", _getpid(),
		 screenshotIndex);

	if(evas_object_image_save(img, dstpath, NULL, "compress=5") != 0)
	{
		if (screenshot_path != NULL) {
			strcpy(screenshot_path, dstpath);
		} else {
			ui_viewer_log("ERROR: capture_object : wrong screenshot_path\n");
			ret = -1;
		}
	}
	evas_object_del(img);

	return ret;
}

int ui_viewer_capture_screen(char *screenshot_path, Evas_Object *obj)
{
	static pthread_mutex_t captureScreenLock = PTHREAD_MUTEX_INITIALIZER;
	int width, height, view_w, view_h;
	int obj_x, obj_y, obj_w, obj_h;
	int ret = 0;
	Evas *evas;
	Evas_Render_Op render_op;
	Eina_Bool visible;
	Evas_Object *parent, *clip;
	Evas_Object *above, *below;
//	Evas_Object *parent_above, *parent_below;
	const char *type_name;
	double k;

	pthread_mutex_lock(&captureScreenLock);

	evas = evas_object_evas_get(obj);
	evas_output_viewport_get(evas, NULL, NULL, &view_w, &view_h);
	evas_object_geometry_get(obj, &obj_x, &obj_y, &obj_w, &obj_h);

	k = ((double)view_w / obj_w < (double)view_h / obj_h) ? (double)view_w / obj_w : (double)view_h / obj_h;

	width = (k < 1.0) ? (int)(obj_w * k) : obj_w;
	height = (k < 1.0) ? (int)(obj_h * k) : obj_h;

	if (width == 0 || height == 0) {
		ui_viewer_log("captureScreen : wrong object geometry [%d,%d]\n",
			      width, height);
		ret = -1;
		goto finish;
	}

	type_name = evas_object_type_get(obj);

	if (!strcmp(type_name, "edje") ||
	    !strcmp(type_name, "elm_win")) {
		ret = captureScreen(screenshot_path, width, height);
		goto finish;
	} else if (!strcmp(type_name, "rectangle") ||
	    !strcmp(type_name, "image")) {
		ret = capture_object(screenshot_path, width, height, obj);
		goto finish;
	} else if (!strcmp(type_name, "vectors")) {
		if (screenshot_path != NULL) {
			screenshot_path[0] = '\0';
		} else {
			ui_viewer_log("ERROR: ui_viewer_capture_screen : wrong screenshot_path\n");
			ret = -1;
		}
		goto finish;
	} else if (!strcmp(type_name, "elm_image") ||
	    !strcmp(type_name, "elm_icon")) {
		Evas_Object *internal_img;
		int img_w, img_h;
		double img_k;

		internal_img = elm_image_object_get(obj);
		evas_object_geometry_get(internal_img, 0, 0, &img_w, &img_h);

		img_k = ((double)view_w / img_w < (double)view_h / img_h) ? (double)view_w / img_w : (double)view_h / img_h;

		img_w = (img_k < 1.0) ? (int)(img_w * img_k) : img_w;
		img_h = (img_k < 1.0) ? (int)(img_h * img_k) : img_h;

		ret = capture_object(screenshot_path, img_w, img_h, internal_img);
		goto finish;
	}

	render_op = evas_object_render_op_get(obj);
	visible = evas_object_visible_get(obj);
	above = evas_object_above_get(obj);
	below = evas_object_below_get(obj);
	clip = evas_object_clip_get(obj);
	parent = evas_object_smart_parent_get(obj);
//	parent_above = evas_object_above_get(parent);
//	parent_below = evas_object_below_get(parent);

	evas_object_clip_unset(obj);
	evas_object_geometry_set(obj, 0, 0, width, height);
	evas_object_render_op_set(obj, EVAS_RENDER_COPY);
	evas_object_raise(parent);
	evas_object_raise(obj);
	evas_object_show(obj);
	evas_render(evas);

	// wait for screen updating
//	poll(0, 0, 100);

	ret = captureScreen(screenshot_path, width, height);

	evas_object_geometry_set(obj, obj_x, obj_y, obj_w, obj_h);
	evas_object_render_op_set(obj, render_op);
/*	if (parent_below != NULL)
		evas_object_stack_above(parent, parent_below);
	if (parent_above != NULL)
		evas_object_stack_below(parent, parent_above);
*/	if (below != NULL)
		evas_object_stack_above(obj, below);
	if (above != NULL)
		evas_object_stack_below(obj, above);
	if (clip != NULL)
		evas_object_clip_set(obj, clip);
	if (!visible)
		evas_object_hide(obj);
	evas_render(evas);
finish:
	pthread_mutex_unlock(&captureScreenLock);
	return ret;
}
