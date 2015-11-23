#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <sys/select.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include "swap_config.h"
#include "debug.h"
#include "buffer.h"

#define TRACE_FILENAME "/trace_offline.bin"

struct daemon_fds {
	int buf_fd;
	int conf_fd;
	int trace_fd;
	int self_pipe_fd[2];
	int swap_fd_pipe[2];
};

static struct daemon_fds da_fds;

int init_daemon_dfs(struct daemon_fds *d_fds)
{
	int saved_flags;

	/* Open swap buffer  */
	d_fds->buf_fd = open(BUF_FILENAME, O_RDONLY | O_CLOEXEC);
	if (d_fds->buf_fd == -1) {
		log_errno("Open swap buffer failed", errno);
		return -1;
	}

	saved_flags = fcntl(d_fds->buf_fd, F_GETFL);
	if (fcntl(d_fds->buf_fd, F_SETFL, saved_flags & ~O_NONBLOCK) == -1) {
		log_errno("fcntl-F_SETFL failed", errno);
		return -1;
	}

	/* Open swap configuretion  */
	d_fds->conf_fd = open(CONF_FILENAME, O_RDONLY);
	if (d_fds->conf_fd  == -1) {
		log_errno("Open swap config failed", errno);
		return -1;
	}

	d_fds->trace_fd = open(TRACE_FILENAME, O_WRONLY | O_CREAT | O_TRUNC,
			       S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
	if (d_fds->trace_fd == -1) {
		log_errno("Open trace file failed", errno);
		return -1;
	}

	/* Open self pipe for handling signals  */
	if (pipe(d_fds->self_pipe_fd) == -1) {
		log_errno("Open pipe failed", errno);
		return -1;
	}

	saved_flags = fcntl(d_fds->self_pipe_fd[0], F_GETFL);
	if (saved_flags == -1) {
		log_errno("fcntl-F_GETFL failed", errno);
		return -1;
	}

	/* Make read end nonblocking  */
	saved_flags |= O_NONBLOCK;
	if (fcntl(d_fds->self_pipe_fd[0], F_SETFL, saved_flags) == -1) {
		log_errno("fcntl-F_SETFL failed", errno);
		return -1;
	}

	saved_flags = fcntl(d_fds->self_pipe_fd[1], F_GETFL);
	if (saved_flags == -1) {
		log_errno("fcntl-F_GETFL failed", errno);
		return -1;
	}

	/* Make write end nonblocking  */
	saved_flags |= O_NONBLOCK;
	if (fcntl(d_fds->self_pipe_fd[1], F_SETFL, saved_flags) == -1) {
		log_errno("fcntl-F_SETFL failed", errno);
		return -1;
	}

	/* Open swap pipe  */
	if (pipe(d_fds->swap_fd_pipe) == -1) {
		log_errno("Open pipe failed", errno);
		return -1;
	}
	return 0;
}

void close_daemon_fds (struct daemon_fds *d_fds)
{
	close(d_fds->self_pipe_fd[0]);
	close(d_fds->self_pipe_fd[1]);
	close(d_fds->swap_fd_pipe[0]);
	close(d_fds->swap_fd_pipe[1]);
	close(d_fds->conf_fd);
	close(d_fds->buf_fd);
	close(d_fds->trace_fd);
}

static void *transfer_thread(void *arg)
{
	int errsv;
	ssize_t nrd, nwr;

	while (1) {
		nrd = splice(da_fds.buf_fd, NULL,
			     da_fds.swap_fd_pipe[1], NULL,
			     BUF_SIZE, 0);
		if (nrd == -1) {
			errsv = errno;
			if (errsv == EAGAIN) {
				log_debmsg("No more data to read\n");
				break;
			}
			errno_exit("Cannot splice read", errno);
		}

		nwr = splice(da_fds.swap_fd_pipe[0], NULL,
			     da_fds.trace_fd, NULL,
			     nrd, 0);

		if (nwr == -1) {
			errsv = errno;
			errno_exit("Cannot splice write", errno);
		}
		if (nwr != nrd) {
			log_debmsg(stderr, "nrd - nwr = %d\n", nrd - nwr);
		}
	}

	return NULL;
}

static void term_handler(int sig) {
	int errsv;

	errsv = errno;
	if (write(da_fds.self_pipe_fd[1], "T", 1) == -1 && errno != EAGAIN) {
		const char *msg = "Write to pipe failed\n";
		write(1, msg, strlen (msg));
		_exit(EXIT_FAILURE);
	}
	errno = errsv;
}

int main()
{
	int saved_flags, ret;
	int ready, nfds;
	struct sigaction sa;
	fd_set read_fds;
	pthread_t transfer_pthread;
	char ch;

	if (initialize_log() == -1) {
		log_errmsg("Failed to initialize log\n");
		exit(EXIT_FAILURE);
	}

	if (init_daemon_dfs(&da_fds) == -1 )
		exit(EXIT_FAILURE);

	if (init_buf(da_fds.buf_fd) == -1 )
		exit(EXIT_FAILURE);

	if (swap_send_conf(da_fds.buf_fd, da_fds.conf_fd))
		exit(EXIT_FAILURE);

	if (daemon(0, 1) == -1)
		errno_exit("Daemonize failed", errno);

	ret = pthread_create(&transfer_pthread, NULL,
			     transfer_thread, NULL);
	if (ret != 0)
		errno_exit("Failed to create transfer thread", ret);

	/* Register SIGTERM termination handler  */
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGINT);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = term_handler;
	if (sigaction(SIGTERM, &sa, NULL) == -1)
		errno_exit ("sigaction failed", errno);

	/* Register SIGINT termination handler  */
	sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, SIGTERM);
        sa.sa_flags = SA_RESTART;
        sa.sa_handler = term_handler;
        if (sigaction(SIGINT, &sa, NULL) == -1)
                errno_exit("sigaction failed", errno);

	FD_ZERO(&read_fds);
	FD_SET(da_fds.self_pipe_fd[0], &read_fds);
	nfds = da_fds.self_pipe_fd[0] + 1;

	/* Restart if interrupted by signal */
	while ((ready = select(nfds, &read_fds, NULL, NULL, NULL)) == -1 &&
		errno == EINTR)
		continue;

	if (ready == -1)
		errno_exit("select failed", errno);

	/* Handler was called */
	if (FD_ISSET(da_fds.self_pipe_fd[0], &read_fds)) {
		for (;;) {
			if (read(da_fds.self_pipe_fd[0], &ch, 1) == -1) {
				if (errno == EAGAIN) {
					/* No more bytes */
					break;
				} else {
					/* Some other error */
					errno_exit("pipe read failed", errno);
				}
			}
		}
		/* Stop buffer  */
		uint32_t stop_msg [2] = {NMSG_STOP, 0};
		if (ioctl_send_msg(da_fds.buf_fd,
				   (const char *) stop_msg) == -1)
			exit(EXIT_FAILURE);

		/* Flush buffer before daemon termination  */
		if (flush_buf(da_fds.buf_fd) == -1)
			exit(EXIT_FAILURE);

		/* Make buffer reading nonblocking  */
		saved_flags = fcntl(da_fds.buf_fd, F_GETFL);
		if (fcntl(da_fds.buf_fd, F_SETFL,
			  saved_flags | O_NONBLOCK) == -1)
			errno_exit("fcntl-F_SETFL failed", errno);

		if (wake_up_buf(da_fds.buf_fd))
			exit(EXIT_FAILURE);

		ret = pthread_join(transfer_pthread, NULL);
		if (ret != 0)
			errno_exit("fcntl-F_SETFL failed", ret);

		if (exit_buf(da_fds.buf_fd) == -1)
			exit(EXIT_FAILURE);
	}

	close_daemon_fds(&da_fds);

	return 0;
}
