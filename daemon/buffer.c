#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "daemon.h"
#include "buffer.h"
#include "ioctl_commands.h"
#include "debug.h"

#define SUBBUF_SIZE 64 * 1024
#define SUBBUF_NUM 16

static int open_buf(void)
{
	manager.buf_fd = open(BUF_FILENAME, O_RDWR);
	if (manager.buf_fd == -1) {
		LOGE("Cannot open buffer: %s\n", strerror(errno));
		return 1;
	}
	LOGI("buffer opened: %s, %d\n", BUF_FILENAME, manager.buf_fd);

	return 0;
}

static void close_buf(void)
{
	LOGI("close buffer (%d)\n", manager.buf_fd);
	close(manager.buf_fd);
}

static int insert_buf_modules(void)
{
	if (system("insmod /opt/swap/sdk/swap_buffer.ko")) {
		LOGE("Cannot insert swap buffer module\n");
		return -1;
	}

	if (system("insmod /opt/swap/sdk/swap_driver.ko")) {
		LOGE("Cannot insert swap driver module\n");
		return -1;
	}

	if (system("insmod /opt/swap/sdk/swap_message_parser.ko")) {
		LOGE("Cannot insert swap message parser module\n");
		return -1;
	}

	return 0;
}

static void remove_buf_modules(void)
{
	LOGI("rmmod parser");
	if (system("rmmod swap_message_parser")) {
		LOGW("Cannot remove swap message parser module\n");
	}

	LOGI("rmmod driver");
	if (system("rmmod swap_driver")) {
		LOGW("Cannot remove swap driver module\n");
	}

	LOGI("rmmod buffer");
	if (system("rmmod swap_buffer")) {
		LOGW("Cannot remove swap buffer module\n");
	}
}

int init_buf(void)
{
	struct buffer_initialize init = {
		.size = SUBBUF_SIZE,
		.count = SUBBUF_NUM,
	};

	if (insert_buf_modules() != 0) {
		LOGE("Cannot insert buffer modules\n");
		return 1;
	}

	if (open_buf() != 0) {
		LOGE("Cannot open buffer\n");
		remove_buf_modules();
		return 1;
	}

	if (ioctl(manager.buf_fd, SWAP_DRIVER_BUFFER_INITIALIZE, &init) == -1) {
		LOGE("Cannot init buffer: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

void exit_buf(void)
{
	LOGI("Uninit buffer (%d)\n", manager.buf_fd);
	if (ioctl(manager.buf_fd, SWAP_DRIVER_BUFFER_UNINITIALIZE) == -1)
		LOGW("Cannot uninit buffer: %s\n", strerror(errno));

	close_buf();
	remove_buf_modules();
}

void flush_buf(void)
{
	if (ioctl(manager.buf_fd, SWAP_DRIVER_FLUSH_BUFFER) == -1)
		LOGW("Cannot flush buffer: %s\n", strerror(errno));
}

int write_to_buf(struct msg_data_t *msg)
{
	if (write(manager.buf_fd, msg, MSG_DATA_HDR_LEN + msg->len) == -1) {
		LOGE("write to buf: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

