#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "daemon.h"
#include "buffer.h"
#include "transfer_thread.h"

#define BUF_SIZE 4096

static void *transfer_thread(void *arg)
{
	(void)arg;
	int fd_pipe[2];
	ssize_t nrd, nwr;

	LOGI("transfer thread started\n");

	pipe(fd_pipe);
	while (1) {
		nrd = splice(manager.buf_fd, NULL,
			     fd_pipe[1], NULL,
			     BUF_SIZE, 0);
		if (nrd == -1) {
			if (errno == EAGAIN) {
				LOGI("No more data to read\n");
				break;
			}
			LOGE("Cannot splice read: %s\n", strerror(errno));
			return NULL;
		}

		nwr = splice(fd_pipe[0], NULL,
			     manager.host.data_socket, NULL,
			     nrd, 0);
		if (nwr == -1) {
			LOGE("Cannot splice write: %s\n", strerror(errno));
			return NULL;
		}
		if (nwr != nrd) {
			LOGW("nrd - nwr = %d\n", nrd - nwr);
		}
	}
	close(fd_pipe[0]);
	close(fd_pipe[1]);

	LOGI("transfer thread finished\n");

	return NULL;
}

int start_transfer()
{
	int saved_flags;

	if (manager.host.data_socket == -1) {
		LOGW("won't start transfer thread: data socket isn't open\n");
		return 0;
	}

	if (manager.transfer_thread != -1) { // already started
		LOGW("transfer already running\n");
		stop_transfer();
	}

	saved_flags = fcntl(manager.buf_fd, F_GETFL);
	fcntl(manager.buf_fd, F_SETFL, saved_flags & ~O_NONBLOCK);

	if(pthread_create(&(manager.transfer_thread),
			  NULL,
			  transfer_thread,
			  NULL) < 0)
	{
		LOGE("Failed to create transfer thread\n");
		return -1;
	}

	return 0;
}

void stop_transfer()
{
	int saved_flags;

	if (manager.transfer_thread == -1) {
		LOGI("transfer thread not running\n");
		return;
	}
	LOGI("stopping transfer\n");

	flush_buf();
	saved_flags = fcntl(manager.buf_fd, F_GETFL);
	fcntl(manager.buf_fd, F_SETFL, saved_flags | O_NONBLOCK);
	pthread_join(manager.transfer_thread, NULL);
	manager.transfer_thread = -1;

	LOGI("transfer thread joined\n");
}
