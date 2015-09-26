/*
 *  DA manager
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Ruslan Soloviev <r.soloviev@samsung.com>
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
 * - Samsung Research Russia
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libwebsockets.h>
#include <errno.h>

#ifdef OLD_JSON
#include <json/json.h>
#else /* OLD_JSON */
#include <json.h>
#endif /* OLD_JSON */


#include "wsi.h"
#include "swap_debug.h"
#include "da_protocol.h"
#include "ioctl_commands.h"
#include "smack.h"
#include "wsi_prof.h" /* Generated automatically */

#define MAX_MSG_LENGTH				4096

#define PSTATE_DEFAULT				(0)
#define PSTATE_CONNECTED			(1L << ( 0))
#define PSTATE_PROFILING			(1L << ( 1))
#define PSTATE_WAIT_ACK				(1L << ( 2))
#define PSTATE_TIMEOUT				(1L << ( 3))
#define PSTATE_START				(1L << ( 4))
#define PSTATE_DISCONNECT			(1L << ( 5))
#define PSTATE_INIT_START			(1L << ( 6))
#define PSTATE_INIT_DONE			(1L << ( 7))

#define SETSTAT(S, F)				((S) |= (F))
#define CLRSTAT(S, F)				((S) &= ~(F))
#define CHKSTAT(S, F)				(((S) & (F)) ? 1: 0)


#define LOG_STATUS LOGW(">>>>> state : %s %s %s %s %s %s\n",\
	CHKSTAT(pstate, PSTATE_DEFAULT)?"PSTATE_DEFAULT":"",\
	CHKSTAT(pstate, PSTATE_CONNECTED)?"PSTATE_CONNECTED":"",\
	CHKSTAT(pstate, PSTATE_PROFILING)?"PSTATE_PROFILING":"",\
	CHKSTAT(pstate, PSTATE_WAIT_ACK)?"PSTATE_WAIT_ACK":"",\
	CHKSTAT(pstate, PSTATE_TIMEOUT)?"PSTATE_TIMEOUT":"",\
	CHKSTAT(pstate, PSTATE_START)?"PSTATE_START":"",\
	CHKSTAT(pstate, PSTATE_DISCONNECT)?"PSTATE_DISCONNECT":"")


enum protocol_names {
	PROTOCOL_PROFILING
};

const char WSI_HOST[] = "127.0.0.1";
struct libwebsocket_context *context;
struct libwebsocket *wsi;

int pstate = PSTATE_DEFAULT;
int request_id = 1;

pthread_t wsi_start_thread = -1;
pthread_t wsi_handle_thread = -1;

static void wsi_destroy(void);

static int set_profile_info(const char *path, const char *info)
{
	FILE *fp;
	int ret = 0, c = 0;

	fp = fopen(path, "w");
	if (fp != NULL) {
		c = fprintf(fp, "%s", info);
		if (c < 0) {
			LOGE("Can't write to file: %s\n", path);
			ret = -EIO;
		}
		fclose(fp);
	} else {
		LOGE("Can't open file: %s\n", path);
		ret = -ENOENT;
	}

	return ret;
}

int wsi_set_profile(const struct app_info_t *app_info)
{
	const char INSPSERVER_START_FILE[] =
		"/sys/kernel/debug/swap/webprobe/inspector_server_start";
	const char WILL_EXECUTE_FILE[] =
		"/sys/kernel/debug/swap/webprobe/will_execute";
	const char DID_EXECUTE_FILE[] =
		"/sys/kernel/debug/swap/webprobe/did_execute";
	const char APP_INFO_FILE[] =
		"/sys/kernel/debug/swap/webprobe/app_info";
	int ret = 0;
	char info_tmp[256] = "";

	ret = snprintf(info_tmp, sizeof(info_tmp), "%s %s", app_info->exe_path,
		 app_info->app_id);

	if (ret < 0 || ret >= sizeof(info_tmp)) {
		LOGE("Can't save app_info, ret = %d\n", ret);
		goto fail;
	}

	ret = set_profile_info(APP_INFO_FILE, info_tmp);
	if (ret)
		goto fail;

	sprintf(info_tmp, "0x%lx", INSPECTOR_ADDR);
	ret = set_profile_info(INSPSERVER_START_FILE, info_tmp);
	if (ret)
		goto fail;

	sprintf(info_tmp, "0x%lx", WILLEXECUTE_ADDR);
	ret = set_profile_info(WILL_EXECUTE_FILE, info_tmp);
	if (ret)
		goto fail;

	sprintf(info_tmp, "0x%lx", DIDEXECUTE_ADDR);
	ret = set_profile_info(DID_EXECUTE_FILE, info_tmp);
	if (ret)
		goto fail;

	return ret;
fail:
	return -EIO;
}

int wsi_set_smack_rules(const struct app_info_t *app_info)
{
	const char SUBJECT[] = "swap";
	const char ACCESS_TYPE[] = "rw";
	const char delim[] = ".";
	int ret = 0;
	char *app_id;
	char *package_id;
	size_t id_maxlen = 128;

	app_id = strndup(app_info->app_id, id_maxlen);
	package_id = strtok(app_id, delim);

	if (package_id != NULL) {
		ret = apply_smack_rules(SUBJECT, package_id, ACCESS_TYPE);
	} else {
		LOGE("Can't determine package id\n");
		ret = -EINVAL;
	}

	free(app_id);
exit:
	return ret;
}

int wsi_enable_profiling(enum web_prof_state_t mode)
{
	const char ENABLED_FILE[] = "/sys/kernel/debug/swap/webprobe/enabled";
	int ret = 0;

	switch (mode) {
	case PROF_ON:
		ret = set_profile_info(ENABLED_FILE, "1");
		if (ret)
			LOGE("Can't enable profiling\n");
		break;
	case PROF_OFF:
		ret = set_profile_info(ENABLED_FILE, "0");
		if (ret)
			LOGE("Can't disable profiling\n");
		break;
	default:
		ret = -EINVAL;
		LOGE("Can't enable or disable profiling\n");
	}

	return ret;
}

static void send_request(const char *method)
{
#define	MAX_REQUEST_LENGTH	128

	json_object *jobj = NULL;
	char buf[LWS_SEND_BUFFER_PRE_PADDING + MAX_REQUEST_LENGTH +
		 LWS_SEND_BUFFER_POST_PADDING];
	const char *payload;

	jobj = json_object_new_object();
	if (jobj == NULL) {
		LOGE("cannot create json object\n");
		return;
	}

	memset(&buf[LWS_SEND_BUFFER_PRE_PADDING], 0, MAX_REQUEST_LENGTH);

	json_object_object_add(jobj, "id", json_object_new_int(request_id++));
	json_object_object_add(jobj, "method", json_object_new_string(method));

	payload = json_object_to_json_string(jobj);
	if (payload == NULL) {
		LOGE("cannot parse json object (method: %s)\n", method);
		return;
	}

	if (strnlen(payload, MAX_REQUEST_LENGTH) != MAX_REQUEST_LENGTH)
		LOGI("json generated len: %d; msg: %s\n", strlen(payload), payload);
	else
		LOGE("bad payload!!!\n");

	strncpy(buf, payload, MAX_REQUEST_LENGTH - 1);

	if (libwebsocket_write(wsi, (unsigned char *)buf, strnlen(payload, MAX_REQUEST_LENGTH - 1) + 1,
			       LWS_WRITE_TEXT) < 0) {
		LOGE("cannot write to web socket (method: %s)\n", method);
		return;
	}

	SETSTAT(pstate, PSTATE_WAIT_ACK);
}

static int profiling_callback(struct libwebsocket_context *context,
			      struct libwebsocket *wsi,
			      enum libwebsocket_callback_reasons reason,
			      void *user, void *in, size_t len)
{
	json_object *jobj, *jobjr;
	int res_id;

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		LOGI("Connected to server\n");
		SETSTAT(pstate, PSTATE_CONNECTED);

		send_request("Profiler.enable");
		send_request("Profiler.start");
		break;

	case LWS_CALLBACK_DEL_POLL_FD:
		LOGE("LWS_CALLBACK_DEL_POLL_FD\n");
	case LWS_CALLBACK_CLOSED:
		LOGI("Connection closed\n");
		CLRSTAT(pstate, PSTATE_CONNECTED);
		SETSTAT(pstate, PSTATE_DISCONNECT);
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		if (libwebsockets_remaining_packet_payload(wsi))
			LOGE("json message too long\n");

		((char *)in)[len] = '\0';
		LOGI("json message recv; len: %d; msg: %s\n", (int)len,
		     (char *)in);

		if (!libwebsocket_is_final_fragment(wsi))
			break;

		jobjr = json_tokener_parse((char *)in);

		/* {"result":{},"id":?} */
		jobj = json_object_object_get(jobjr, "id");
		if (json_object_object_get(jobjr, "result") && jobj) {
			res_id = json_object_get_int(jobj);
			if (res_id == request_id - 1) {
				CLRSTAT(pstate, PSTATE_WAIT_ACK);
				if (CHKSTAT(pstate, PSTATE_DISCONNECT))
					wsi_destroy();
			}
			break;
		}

		/*
		 * { "method":"Profiler.setRecordingProfile",
		 * "params":{"isProfiling":true|false}}
		 */
		while (1) {
			const char *s1, s2[] = "Profiler.setRecordingProfile";

			jobj = json_object_object_get(jobjr, "method");
			if (!jobj)
				break;
			s1 = json_object_get_string(jobj);
			if (s1 && strncmp(s1, s2, sizeof(s2)))
				break;
			jobj = json_object_object_get(jobjr, "params");
			if (!jobj)
				break;
			jobj = json_object_object_get(jobj, "isProfiling");
			if (!jobj)
				break;
			if (json_object_get_boolean(jobj)) {
				SETSTAT(pstate, PSTATE_PROFILING);
				CLRSTAT(pstate, PSTATE_START);
			} else {
				CLRSTAT(pstate, PSTATE_PROFILING);
			}
			break;
		}

		break;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		LOGE("Web socket connection error. Falling down\n");
		/*
		 * TODO: need report to host about error. May be should stop
		 * web profiling and/or launching web application
		 */
		break;

	default:
		LOGW("Message unknown. Reason: %d\n", reason);
		break;
	}

	return 0;
}

static struct libwebsocket_protocols protocols[] = {
	{ "profiling-protocol", profiling_callback, 0, MAX_MSG_LENGTH },
	{ NULL, NULL, 0, 0 }
};

static void destroy_wsi_conn(struct libwebsocket_context *context)
{
	libwebsocket_context_destroy(context);
}

static int init_wsi_conn(struct libwebsocket_context **context,
			 struct libwebsocket **wsi,
			 const char *address, int port)
{
	static struct lws_context_creation_info info;
	char host[32];
	char origin[32];
	const char *page = "/devtools/page/1";
	int ietf_version = -1; /* latest */

	sprintf(host, "%s:%d", address, port);
	sprintf(origin, "http://%s:%d", address, port);
	LOGI(" host =<%s> origin = <%s>\n", host, origin);

	memset(&info, 0, sizeof(info));

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.extensions = libwebsocket_get_internal_extensions();
	info.options = 0;

	*context = libwebsocket_create_context(&info);
	if (*context == NULL) {
		LOGE("libwebsocket context creation failed\n");
		return 1;
	}

	*wsi = libwebsocket_client_connect(*context, address, port, 0, page,
					   host, origin,
					   protocols[PROTOCOL_PROFILING].name,
					   ietf_version);

	if (*wsi == NULL) {
		LOGE("libwebsocket connect failed\n");
		destroy_wsi_conn(*context);
		return 1;
	}

	LOGI("Websocket connection opened\n");

	return 0;
}

static void *handle_ws_responses(void *arg)
{
	while (CHKSTAT(pstate, PSTATE_START | PSTATE_PROFILING |
		       PSTATE_WAIT_ACK) && !CHKSTAT(pstate, PSTATE_DISCONNECT)) {
			LOG_STATUS;

		int res = libwebsocket_service(context, 1000);
	}

	LOGI("handle response thread finished\n");
	return NULL;
}

static int wsi_init(const char *address, int port)
{
	int res = 0;
	if (!port) {
		char buf[sizeof(struct msg_t) + sizeof(uint32_t)];
		struct msg_t *msg = (struct msg_t *)buf;

		msg->id = NMSG_WRT_LAUNCHER_PORT;
		msg->len = sizeof(uint32_t);
		if (!ioctl_send_msg(msg))
			port = *(int *)&msg->payload;
	}

	if (port) {
		LOGI("wrt-launcher debug port: %d\n", port);
	} else {
		LOGE("wrt-launcher debug port not exist\n");
		res = 1;
		goto exit;
	}

	if (init_wsi_conn(&context, &wsi, address, port) != 0) {
		LOGE("cannot init wsi connection\n");
		res = 1;
		goto exit;
	}

exit:
	return res;
}

static void wsi_destroy(void)
{
	if (CHKSTAT(pstate, PSTATE_CONNECTED)) {
		if (CHKSTAT(pstate, PSTATE_WAIT_ACK)) {
			SETSTAT(pstate, PSTATE_DISCONNECT);
		} else {
			LOGI("destroy context\n");
			destroy_wsi_conn(context);
		}
	} else {
		LOGW("Try disconnect when web socket not connected\n");
	}
}

static int wsi_start_profiling(void)
{
	if (pthread_create(&wsi_handle_thread, NULL, &handle_ws_responses, NULL)) {
		LOGE("Can't create handle_ws_ threads\n");
		destroy_wsi_conn(context);
		return 1;
	}

	return 0;
}

static void wsi_stop_profiling(void)
{
	if (CHKSTAT(pstate, PSTATE_CONNECTED)) {
		send_request("Profiler.stop");
		send_request("Profiler.disable");
	}
}

static void *do_start(void *arg)
{
	LOGI("wsi thread started\n");

	if (wsi_init(WSI_HOST, 0)) {
		LOGE("Cannot init web application profiling\n");
		pstate = PSTATE_DEFAULT;
		goto out;
	}

	if (wsi_start_profiling()) {
		LOGE("Cannot start web application profiling\n");
		pstate = PSTATE_DEFAULT;
		goto out;
	}

	SETSTAT(pstate, PSTATE_INIT_DONE);

out:
	LOGI("wsi thread finished\n");
	return NULL;
}

int wsi_start(void)
{
	int ret;
	SETSTAT(pstate, PSTATE_INIT_START);

	ret = pthread_create(&wsi_start_thread, NULL, &do_start, NULL);
	if (ret) {
		LOGE("Can't create do_start thread\n");
		wsi_start_thread = -1;
		pstate = PSTATE_DEFAULT;
	}

	return ret;
}

void wsi_stop(void)
{
	void *thread_ret = NULL;

	if (CHKSTAT(pstate, PSTATE_INIT_START)) {
		/* init was start */
		while (!CHKSTAT(pstate, PSTATE_INIT_DONE) && !CHKSTAT(pstate, PSTATE_DEFAULT)) {
			LOGW("stop request while init not finished. wait.\n");
			sleep(1);
		}

		if (CHKSTAT(PSTATE_DEFAULT)) {
			LOGE("wsi init fail.\n");
			return;
		}
	} else {
		/* init has not be started */
		LOGW("Try stop wsi without swi_start call. Ignored.\n");
		return;
	}

	wsi_stop_profiling();

	if (is_feature_enabled(FL_WEB_PROFILING)) {
		if (wsi_enable_profiling(PROF_OFF))
			LOGE("Cannot disable web application profiling\n");
	}

	SETSTAT(pstate, PSTATE_DISCONNECT);

	if (wsi_start_thread != -1)
		pthread_join(wsi_start_thread, &thread_ret);
	wsi_start_thread = -1;


	if (wsi_handle_thread != -1) {
		LOGI("wait handle thread");
		pthread_join(wsi_handle_thread, &thread_ret);
	}
	wsi_handle_thread = -1;

	destroy_wsi_conn(context);

	pstate = PSTATE_DEFAULT;
}
