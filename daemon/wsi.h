
int init_ws_conn(struct libwebsocket_context **context,
		  struct libwebsocket **wsi);
void destroy_ws_conn(struct libwebsocket_context *context);
void handle_ws_requests(void *wsi_ptr);
void handle_ws_responses(void *context_ptr);
