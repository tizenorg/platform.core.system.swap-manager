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
static pthread_mutex_t captureScreenLock = PTHREAD_MUTEX_INITIALIZER;

static char* captureScreenShotX(screenshot_data* sdata)
{
	Window root;
	int width, height;

	sdata->dpy = XOpenDisplay(NULL);
	if(sdata->dpy == NULL)
	{
		ui_viewer_log("ERROR: XOpenDisplay failed\n");
		return NULL;
	}

	width = DisplayWidth(sdata->dpy, DefaultScreen(sdata->dpy));
	height = DisplayHeight(sdata->dpy, DefaultScreen(sdata->dpy));

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

static int capture_object(char *screenshot_path, int width, int height,
			  Evas_Object *obj, const char *type_name) {
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
	} else {
		char* scrimage;
		screenshot_data sdata;
		int x, y;
		int screen_w;
		Evas_Object *image;
		int i, j;
		int bytes_per_pixel;

		evas_object_geometry_get(obj, &x, &y, NULL, NULL);

		sdata.ximage = NULL;
		scrimage = captureScreenShotX(&sdata);
		if (!scrimage) {
			ui_viewer_log("ERROR: capture_object : no scrimage\n");
			ret = -1;
			goto finish;
		}
		bytes_per_pixel = sdata.ximage->bits_per_pixel / 8;
		screen_w = sdata.ximage->width;

		image_data = malloc(width * height * bytes_per_pixel);
		if (!image_data) {
			ui_viewer_log("ERROR: capture_object : can't allocate %d bytes\n",
				      width * height * bytes_per_pixel);
			ret = -1;
			goto finish;
		}

		// crop image
		for (j = 0; j < height; j++) {
			for (i = 0; i < width * bytes_per_pixel; i++) {
				image_data[i + j * width * bytes_per_pixel] =
				scrimage[(i + x * bytes_per_pixel) + (j + y) * screen_w * bytes_per_pixel];
			}
		}

		image = evas_object_image_filled_add(sub_canvas);
		evas_object_image_size_set(image, width, height);
		evas_object_image_data_set(image, image_data);
		evas_object_image_data_update_add(image, 0, 0, width, height);
		evas_object_resize(image, width, height);
		evas_object_show(image);
finish:
		releaseScreenShotX(&sdata);
	}

	ecore_evas_manual_render(sub_ee);

	snprintf(dstpath, sizeof(dstpath), TMP_DIR"/%d_%d.png", _getpid(),
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
		ui_viewer_log("ui_viewer_capture_screen : no screenshot path\n");
		ret = -1;
		return ret;
	}

	pthread_mutex_lock(&captureScreenLock);

	_strncpy(type_name, evas_object_type_get(obj), MAX_PATH_LENGTH);

	evas = evas_object_evas_get(obj);
	evas_output_viewport_get(evas, NULL, NULL, &view_w, &view_h);
	evas_object_geometry_get(obj, &obj_x, &obj_y, &obj_w, &obj_h);

	if (obj_w == 0 || obj_h == 0) {
		ui_viewer_log("ui_viewer_capture_screen : object %p[%d,%d] has zero width or height\n",
			      obj, obj_w, obj_h);
		ret = -1;
	} else if (!strcmp(type_name, "rectangle")) {
		int width, height;

		// restrict rectangle area
		width = (obj_w <= view_w) ? obj_w : view_w;
		height = (obj_h <= view_h) ? obj_h : view_h;

		ret = capture_object(screenshot_path, width, height, obj, type_name);
	} else if (!strcmp(type_name, "image")) {
		ret = capture_object(screenshot_path, obj_w, obj_h, obj, type_name);
	} else if (!strcmp(type_name, "vectors")) {
		ret = -1;
	} else if (!strcmp(type_name, "elm_image") ||
	    !strcmp(type_name, "elm_icon")) {
		Evas_Object *internal_img;
		int img_w, img_h;

		internal_img = elm_image_object_get(obj);
		evas_object_geometry_get(internal_img, NULL, NULL, &img_w, &img_h);

		ret = capture_object(screenshot_path, img_w, img_h, internal_img, type_name);
	} else if (obj_x > view_w || obj_y > view_h) {
		ui_viewer_log("ui_viewer_capture_screen : object %p lies beside view area\n",
			      obj);
		ret = -1;
	} else if (!evas_object_visible_get(obj)) {
		ui_viewer_log("ui_viewer_capture_screen : object %p is unvisible\n",
			      obj);
		ret = -1;
	} else {
		int width, height;

		// take visible on screen part of object
		width = (view_w < obj_w + obj_x) ? (view_w - obj_x) : obj_w;
		height = (view_h < obj_h + obj_y) ? (view_h - obj_y) : obj_h;

		ret = capture_object(screenshot_path, width, height, obj, type_name);
	}

	if (ret)
		screenshot_path[0] = '\0';

	pthread_mutex_unlock(&captureScreenLock);
	return ret;
}
