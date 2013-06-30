#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "daemon.h"
#include "buffer.h"

static int buf_fd = 0;

static int open_buf(void)
{
	buf_fd = creat(BUF_FILENAME, 0644);
	if (buf_fd == -1) {
		LOGE("Cannot open buffer: %s\n", strerror(errno));
		return 1;
	}
	LOGI("buffer opened: %s, %d\n", BUF_FILENAME, buf_fd);

	return 0;
}

static void close_buf(void)
{
	close(buf_fd);
}

static int insert_buf_modules(void)
{
	system("insmod /opt/swap/sdk/swap_buffer.ko");
	system("insmod /opt/swap/sdk/swap_driver.ko");

	return 0;
}

static void remove_buf_modules(void)
{
	system("rmmod /opt/swap/sdk/swap_driver.ko");
	system("rmmod /opt/swap/sdk/swap_buffer.ko");
}

int init_buf(void)
{
	if (insert_buf_modules() != 0) {
		LOGE("Cannot insert buffer modules\n");
		return 1;
	}

	if (open_buf() != 0) {
		LOGE("Cannot open buffer\n");
		remove_buf_modules();
		return 1;
	}

	return 0;
}

void exit_buf(void)
{
	close_buf();
	remove_buf_modules();
}

int write_to_buf(struct msg_data_t *msg)
{
	if (write(buf_fd, msg, MSG_DATA_HDR_LEN + msg->len) == -1) {
		LOGE("write to buf: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}
