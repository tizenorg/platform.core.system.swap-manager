#ifndef __BUFFER__
#define __BUFFER__

#include <linux/ioctl.h>

#define SWAP_DRIVER_IOC_MAGIC 0xAF
#define SWAP_DRIVER_BUFFER_INITIALIZE           _IOW(SWAP_DRIVER_IOC_MAGIC, 1, \
                                                     struct buffer_initialize *)
#define SWAP_DRIVER_BUFFER_UNINITIALIZE         _IO(SWAP_DRIVER_IOC_MAGIC, 2)
#define SWAP_DRIVER_NEXT_BUFFER_TO_READ         _IO(SWAP_DRIVER_IOC_MAGIC, 3)
#define SWAP_DRIVER_FLUSH_BUFFER                _IO(SWAP_DRIVER_IOC_MAGIC, 4)
#define SWAP_DRIVER_MSG                         _IOW(SWAP_DRIVER_IOC_MAGIC, 5, \
                                                     void *)
#define SWAP_DRIVER_WAKE_UP                     _IO(SWAP_DRIVER_IOC_MAGIC, 6)

#define NMSG_STOP 0x0003
#define BUF_SIZE 4096
#define SUBBUF_SIZE 32 * 1024
#define SUBBUF_NUM 8

#define BUF_FILENAME "/dev/swap_device"


int init_buf(int buf_fd);
int flush_buf(int buf_fd);
int wake_up_buf(int buf_fd);
int ioctl_send_msg(int buf_fd, const char *msg);
int exit_buf(int buf_fd);

#endif //__BUFFER__
