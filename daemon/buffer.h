#ifndef _BUFFER_
#define _BUFFER_

#include "da_protocol.h"

#define BUF_FILENAME "/dev/swap_device"

int init_buf(void);
void exit_buf(void);
void flush_buf(void);
int write_to_buf(struct msg_data_t *msg);

#endif /* _BUFFER_ */
