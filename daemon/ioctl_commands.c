/* SWAP Device ioctl commands */

#include "ioctl_commands.h"
#include "debug.h"
#include "da_protocol.h"
#include "daemon.h"

#include <errno.h>

//send message to device
int ioctl_send_msg(struct msg_data_t *msg)
{
	LOGI("write to device\n");
	if (ioctl(manager.buf_fd, 0, msg) == -1) {
		LOGE("write to device: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}


