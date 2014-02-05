/*
 *  DA manager
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Anastasia Lyupa <a.lyupa@samsung.com>
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
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include <libwebsockets.h>
#include <jansson.h>

#include "wsi.h"
#include "debug.h"

enum { max_msg_length = 4096 };
static int handling = 1;

enum protocol_names {
	PROTOCOL_PROFILING
};

enum frame_states {
	FRAME_START_MESSAGE,
	FRAME_PAYLOAD
};

struct frame_data {
	int total_message;
	enum frame_states state;
};

static void decode_response(char *buf);

static int callback_profiling(struct libwebsocket_context *context,
				struct libwebsocket *wsi,
				enum libwebsocket_callback_reasons reason,
				void *user, void *in, size_t len)
{
	struct frame_data *frd = (struct frame_data *)user;
	char buf[max_msg_length];
	size_t remaining;

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		LOGI("websocket connection opened\n");
		frd->state = FRAME_START_MESSAGE;
		break;
	case LWS_CALLBACK_CLOSED:
		LOGI("websocket connection closed\n");
		break;
	case LWS_CALLBACK_CLIENT_RECEIVE:
		if (frd->state == FRAME_START_MESSAGE) {
			frd->total_message = 0;
			buf[0] = '\0';
			frd->state = FRAME_PAYLOAD;
		}

		((char *)in)[len] = '\0';
		strncat(buf, (char *)in, len);
		frd->total_message += len;
		remaining = libwebsockets_remaining_packet_payload(wsi);

		if (!remaining && libwebsocket_is_final_fragment(wsi)) {
			LOGI("msg: length = %d: %s\n",
				frd->total_message, buf);
			decode_response(buf);
			frd->state = FRAME_START_MESSAGE;
		}
		break;
	default:
		break;
	}

	return 0;
}

static struct libwebsocket_protocols protocols[] = {
	{
		"profiling-protocol",
		callback_profiling,
		sizeof(struct frame_data),
		20
	},
	{
		NULL, NULL, 0   /* End of list */
	}
};

void destroy_ws_conn(struct libwebsocket_context *context)
{
	libwebsocket_context_destroy(context);
}

int init_ws_conn(struct libwebsocket_context **context, struct libwebsocket **wsi)
{
	struct lws_context_creation_info info;
	int port = 11223;
	const char *address = "localhost";
	char host[32];
	char origin[32];
	const char *page = "/devtools/page/1";
	int ietf_version = -1; /* latest */

	sprintf(host, "%s:%d", address, port);
	sprintf(origin, "http://%s:%d", address, port);

	memset(&info, 0, sizeof info);

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
		LOGE("websocket context creation failed\n");
		return 1;
	}

	*wsi = libwebsocket_client_connect(*context, address, port, 0, page, host,
		origin, protocols[PROTOCOL_PROFILING].name, ietf_version);

	if (*wsi == NULL) {
		LOGE("websocket connection failed\n");
		destroy_ws_conn(*context);
		return 1;
	}

	return 0;
}

static void send_request(struct libwebsocket *wsi, int *id,
			 const char *method, json_t *params)
{
	char *payload;
	json_t *request;
	char buf[LWS_SEND_BUFFER_PRE_PADDING + 128
		  + LWS_SEND_BUFFER_POST_PADDING];

	memset(&buf[LWS_SEND_BUFFER_PRE_PADDING], 0, 128);

	request = json_pack("{s:i, s:s}", "id", *id, "method", method);
	if (params != NULL) {
		json_object_set(request, "params", params);
		json_decref(params);
	}

	payload = json_dumps(request, JSON_COMPACT);

	strcpy(buf, payload);

	libwebsocket_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING],
				strlen(payload), LWS_WRITE_TEXT);

	free(payload);
	++(*id);
}

/* Timeline inst */

struct event_record {
	double startTime;
	// TODO: struct data (differs);
	// TODO: children ?
	double endTime;
	const char *type;
	const char *frameId;
	int usedHeapSize;
	int totalHeapSize;
};

static void timeline_start(struct libwebsocket *wsi, int *id,
			   int maxCallStackDepth)
{
	json_t *params;

	params = json_pack("{s:i}", "maxCallStackDepth", maxCallStackDepth);
	send_request(wsi, id, "Timeline.start", params);
}

static void timeline_stop(struct libwebsocket *wsi, int *id)
{
	send_request(wsi, id, "Timeline.stop", NULL);
}

static void perform_inst_session(struct libwebsocket *wsi, int *id)
{
	/* timeline */
	timeline_start(wsi, id, 5);
	sleep(5);
}

void *handle_ws_requests(void *wsi_ptr)
{
	int request_id = 1;

	struct libwebsocket *wsi = (struct libwebsocket *)wsi_ptr;

	perform_inst_session(wsi, &request_id);
	sleep(3);

	handling = 0;
}

static int decode_timeline_event(json_t *response)
{
	int ret = 0;

	struct event_record rec;

	// TODO: get events recursively (children - array of events),
	// we can use json_object_iter
	if (!json_unpack(response, "{s:{s:{s:f,s:f,s:s,s:s,s:i,s:i}}}", "params", "record",
		"startTime", &rec.startTime, "endTime", &rec.endTime, "type", &rec.type,
		"frameId", &rec.frameId, "usedHeapSize", &rec.usedHeapSize,
		"totalHeapSize", &rec.totalHeapSize))
	{
		LOGI("record: startTime: %lf, endTime: %lf, type: %s, frameId: %s, "
			"usedHeapSize: %d, totalHeapSize: %d\n", rec.startTime, rec.endTime,
			rec.type, rec.frameId, rec.usedHeapSize, rec.totalHeapSize);
	} else {
		LOGE("can't unpack timeline event record\n");
		ret = 1;
	}

	return ret;
}

static void decode_response(char *buf)
{
	json_t *response;
	json_error_t error;
	const char *method;

	response = json_loads(buf, 0, &error);

	if (!json_unpack(response, "{s:s}", "method", &method)) {
		if (!strcmp(method, "Timeline.eventRecorded")) {
			decode_timeline_event(response);
		}
	}
}

void *handle_ws_responses(void *context_ptr)
{
	int timeout_ms = 50;

	struct libwebsocket_context *context;
	context = (struct libwebsocket_context *)context_ptr;

	while (handling) {
		libwebsocket_service(context, timeout_ms);
	}
}
/*
// TODO: include it to manager
int main(void) {
	struct libwebsocket_context *context;
	struct libwebsocket *wsi;

	int err[2];
	pthread_t tid[2];

	if (init_ws_conn(&context, &wsi) != 0) {
		return 1;
	};

        err[0] = pthread_create(&(tid[0]), NULL, &handle_ws_requests,
				(void*) wsi);
	err[1] = pthread_create(&(tid[1]), NULL, &handle_ws_responses,
				(void*) context);

	if (err[0] != 0 || err[1] != 0) {
		LOGE("can't create handle_ws_ threads\n");
		destroy_ws_conn(context);
		return 1;
	}

	// wait for threads completing

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);

	// destroy connection
	destroy_ws_conn(context);

	return 0;
}*/
