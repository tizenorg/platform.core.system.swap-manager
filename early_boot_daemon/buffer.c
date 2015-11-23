#include <stdint.h>
#include "debug.h"
#include "buffer.h"

struct buffer_initialize {
	size_t size;
	unsigned int count;
};

int init_buf(int buf_fd) {
        struct buffer_initialize init = {
                .size = SUBBUF_SIZE,
                .count = SUBBUF_NUM,
        };
        if (ioctl(buf_fd, SWAP_DRIVER_BUFFER_INITIALIZE, &init) == -1) {
                log_errno ("Cannot init buffer", errno);
                return -1;
        }
        return 0;
}

int flush_buf(int buf_fd) {
        if (ioctl(buf_fd, SWAP_DRIVER_FLUSH_BUFFER) == -1) {
                log_errno ("Cannot send flush to driver", errno);
                return -1;
        }
        return 0;
}

int wake_up_buf(int buf_fd) {
        if (ioctl(buf_fd, SWAP_DRIVER_WAKE_UP) == -1) {
		log_errno ("Cannot send wake up to driver", errno);
                return -1;
        }
}

int ioctl_send_msg (int buf_fd, const char *msg) {
        if (ioctl (buf_fd, SWAP_DRIVER_MSG, msg) == -1) {
                log_errno ("Write to device failed", errno);
                return -1;
        }
        return 0;
}

int exit_buf(int buf_fd) {
        if (ioctl(buf_fd, SWAP_DRIVER_BUFFER_UNINITIALIZE) == -1) {
		log_errno ("Cannot uninit driver", errno);
		return -1;
        }
	return 0;
}
