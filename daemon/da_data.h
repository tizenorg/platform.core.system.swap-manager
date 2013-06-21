#ifndef _DA_DATA_H_
#define _DA_DATA_H_
int fill_data_msg_head (struct msg_data_t *data, uint32_t msgid, uint32_t seq, uint32_t len);

//allocate memory, need free!!!
int gen_message_terminate(struct msg_data_t *data, uint32_t id);

//allocate memory, need free!!!

int reset_data_msg(struct msg_data_t *data);

int gen_message_error(struct msg_data_t *data, const char * err_msg);

int gen_message_event(struct msg_data_t *data,
		      struct input_event *events,
		      uint32_t events_count,
		      uint32_t id);
int reset_data_msg(struct msg_data_t *data);

#endif //#ifndef _DA_DATA_H_
