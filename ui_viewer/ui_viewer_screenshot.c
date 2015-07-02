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

#define MAX_HEIGHT		720

typedef struct _screenshot_data
{
	XImage*			ximage;
	Display*		dpy;
	XShmSegmentInfo	x_shm_info;
} screenshot_data;

static int screenshotIndex = 0;

static char* captureScreenShotX(int* pwidth, int* pheight, screenshot_data* sdata)
{
	Window root;

	sdata->dpy = XOpenDisplay(NULL);
	if(sdata->dpy == NULL)
	{
		ui_viewer_log("ERROR: XOpenDisplay failed\n");
		return NULL;
	}

	*pwidth = DisplayWidth(sdata->dpy, DefaultScreen(sdata->dpy));
	*pheight = DisplayHeight(sdata->dpy, DefaultScreen(sdata->dpy));

	root = RootWindow(sdata->dpy, DefaultScreen(sdata->dpy));

	sdata->ximage = XShmCreateImage(sdata->dpy, DefaultVisualOfScreen (DefaultScreenOfDisplay (sdata->dpy)), 24,
					ZPixmap, NULL, &sdata->x_shm_info, (unsigned int)*pwidth, (unsigned int)*pheight);

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

int captureScreen(char *screenshot_path)
{
	static pthread_mutex_t captureScreenLock = PTHREAD_MUTEX_INITIALIZER;
	char dstpath[MAX_PATH_LENGTH];
	char* scrimage;
	int width, height;
	Evas* ev = NULL;
	Evas_Object* img;
	screenshot_data sdata;
	int ret = 0;

	pthread_mutex_lock(&captureScreenLock);

	screenshotIndex++;

	sdata.ximage = NULL;
	scrimage = captureScreenShotX(&width, &height, &sdata);
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

				// resize image
				if(height > MAX_HEIGHT)
				{
					width = width * MAX_HEIGHT / height;
					height = MAX_HEIGHT;
					evas_object_resize(img, width, height);
					evas_object_image_fill_set(img, 0, 0, width, height);
				}
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

	pthread_mutex_unlock(&captureScreenLock);
	return ret;
}
