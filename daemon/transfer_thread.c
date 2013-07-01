#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "daemon.h"
#include "buffer.h"

#define BUF_SIZE 4096

static void *transfer_thread(void *arg)
{
	(void)arg;
	char buf[BUF_SIZE];
	int fd_pipe[2];
	ssize_t n;

	LOGI("transfer thread started\n");

	pipe(fd_pipe);
	while (1) {
		LOGI("going to splice read\n");
		n = splice(manager.buf_fd, NULL, fd_pipe[1], NULL, BUF_SIZE, 0);
		if (n == -1) {
			if (errno == EAGAIN) {
				LOGI("No more data to read\n");
				break;
			}
			LOGE("Cannot splice read: %s\n", strerror(errno));
			return 1;
		}
		printf("splice read: %ld\n", n);

		n = splice(fd_pipe[0], NULL,
			   manager.host.data_socket, NULL, n, 0);
		if (n == -1) {
			LOGE("Cannot splice write: %s\n", strerror(errno));
			return 1;
		}
		LOGI("splice written: %ld\n", n);
	}
	close(fd_pipe[0]);
	close(fd_pipe[1]);

	LOGI("transfer thread finished\n");

	return 0;
}

int start_transfer()
{
	if (manager.transfer_thread != -1) // already started
		return 1;

	if(pthread_create(&(manager.transfer_thread),
			  NULL,
			  transfer_thread,
			  NULL) < 0)
	{
		LOGE("Failed to create transfer thread\n");
		return 1;
	}

	return 0;
}

void stop_transfer()
{
	int saved_flags;

	LOGI("stopping transfer\n");

	flush_buf();
	saved_flags = fcntl(manager.buf_fd, F_GETFL);
	fcntl(manager.buf_fd, F_SETFL, saved_flags | O_NONBLOCK);
	pthread_join(manager.transfer_thread, NULL);

	LOGI("transfer thread joined\n");
}