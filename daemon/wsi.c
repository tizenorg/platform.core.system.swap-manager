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
#include <json/json.h>

#include "wsi.h"
#include "debug.h"
#include "da_protocol.h"
#include "ioctl_commands.h"

#define MAX_MSG_LENGTH				4096

#define PSTATE_DEFAULT				(0)
#define PSTATE_CONNECTED			(1L << ( 0))
#define PSTATE_PROFILING			(1L << ( 1))
#define PSTATE_WAIT_ACK				(1L << ( 2))
#define PSTATE_TIMEOUT				(1L << ( 3))
#define PSTATE_START				(1L << ( 4))
#define PSTATE_DISCONNECT			(1L << ( 5))

#define SETSTAT(S, F)				((S) |= (F))
#define CLRSTAT(S, F)				((S) &= ~(F))
#define CHKSTAT(S, F)				(((S) & (F)) ? 1: 0)

enum protocol_names {
	PROTOCOL_PROFILING
};

struct libwebsocket_context *context;
struct libwebsocket *wsi;

int pstate = PSTATE_DEFAULT;
int request_id = 1;

static void send_request(const char *method)
{
#define	MAX_REQUEST_LENGTH	128

	json_object *jobj = json_object_new_object();
	char buf[LWS_SEND_BUFFER_PRE_PADDING + MAX_REQUEST_LENGTH +
		 LWS_SEND_BUFFER_POST_PADDING];
	const char *payload;

	memset(&buf[LWS_SEND_BUFFER_PRE_PADDING], 0, MAX_REQUEST_LENGTH);

	json_object_object_add(jobj, "id", json_object_new_int(request_id++));
	json_object_object_add(jobj, "method", json_object_new_string(method));

	payload = json_object_to_json_string(jobj);
	if (payload == NULL) {
		LOGE("cannot parse json object (method: %s)\n", method);
		return;
	}
	strcpy(buf, payload);

	if (libwebsocket_write(wsi, (unsigned char *)buf, strlen(payload), LWS_WRITE_TEXT) < 0) {
		LOGE("cannot write to web socket (method: %s)\n", method);
		return;
	}

	SETSTAT(pstate, PSTATE_WAIT_ACK);
	LOGI("json message send; len: %d; msg: %s\n", strlen(payload), payload);
}

static int profiling_callback(struct libwebsocket_context *context,
			      struct libwebsocket *wsi,
			      enum libwebsocket_callback_reasons reason,
			      void *user, void *in, size_t len)
{
#define	BUF_SIZE	256

	json_object *jobj, *jobjr;
	char buf[BUF_SIZE];
	const char *str = buf;
	int res_id;

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		LOGI("Connected to server\n");
		SETSTAT(pstate, PSTATE_CONNECTED);

		send_request("Profiler.enable");
		send_request("Profiler.start");
		break;

	case LWS_CALLBACK_CLOSED:
		LOGI("Connection closed\n");
		CLRSTAT(pstate, PSTATE_CONNECTED);
		CLRSTAT(pstate, PSTATE_DISCONNECT);
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		if (libwebsockets_remaining_packet_payload(wsi))
			LOGE("json message too long\n");

		((char *)in)[len] = '\0';
		LOGI("json message recv; len: %d; msg: %s\n", (int)len, (char *)in);

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
			jobj = json_object_object_get(jobjr, "method");
			if (!jobj)
				break;
			str = json_object_get_string(jobj);
			if (strcmp(str, "Profiler.setRecordingProfile"))
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
		LOGE("Web socket connection error\n");
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
	struct lws_context_creation_info info;
	char host[32];
	char origin[32];
	const char *page = "/devtools/page/1";
	int ietf_version = -1; /* latest */

	sprintf(host, "%s:%d", address, port);
	sprintf(origin, "http://%s:%d", address, port);

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
		       PSTATE_WAIT_ACK))
		libwebsocket_service(context, 50);

	return NULL;
}

int wsi_init(const char *address, int port)
{
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
		return 1;
	}

	if (init_wsi_conn(&context, &wsi, address, port) != 0)
		return 1;

	return 0;
}

void wsi_destroy(void)
{
	if (CHKSTAT(pstate, PSTATE_CONNECTED)) {
		if (CHKSTAT(pstate, PSTATE_WAIT_ACK))
			SETSTAT(pstate, PSTATE_DISCONNECT);
		else
			destroy_wsi_conn(context);
	} else {
		LOGW("Try disconnect when web socket not connected\n");
	}
}

int wsi_start_profiling(void)
{
	pthread_t tid;

	SETSTAT(pstate, PSTATE_START);

	if (pthread_create(&tid, NULL, &handle_ws_responses, NULL)) {
		LOGE("Can't create handle_ws_ threads\n");
		destroy_wsi_conn(context);
		return 1;
	}

	return 0;
}

void wsi_stop_profiling(void)
{
	if (CHKSTAT(pstate, PSTATE_CONNECTED)) {
		send_request("Profiler.stop");
		send_request("Profiler.disable");
	}
}
