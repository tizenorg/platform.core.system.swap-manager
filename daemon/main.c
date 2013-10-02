/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim <jaewon81.lim@samsung.com>
 * Woojin Jung <woojin2.jung@samsung.com>
 * Juyoung Kim <j0.kim@samsung.com>
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
 * Nikita Kalyazin    <n.kalyazin@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Contributors:
 * - S-Core Co., Ltd
 * - Samsung RnD Institute Russia
 *
 */

#include <stdio.h>			// for fopen, fprintf
#include <stdlib.h>			// for atexit
#include <sys/types.h>		// for open
#include <sys/file.h>
#include <sys/stat.h>		// for open
#include <sys/socket.h>		// for socket
#include <sys/un.h>			// for sockaddr_un
#include <arpa/inet.h>		// for sockaddr_in, socklen_t

#include <signal.h>			// for signal
#include <unistd.h>			// for unlink
#include <fcntl.h>			// for open, fcntl
#include <stdbool.h>
#ifndef LOCALTEST
#include <attr/xattr.h>		// for fsetxattr
#endif
#include "daemon.h"
#include "da_protocol.h"
#include "sys_stat.h"
#include "buffer.h"
#include "debug.h"

#define SINGLETON_LOCKFILE			"/tmp/da_manager.lock"
#define PORTFILE					"/tmp/port.da"
#define UDS_NAME					"/tmp/da.socket"

#define INIT_PORT				8001
#define LIMIT_PORT				8100


// initialize global variable
__da_manager manager =
{
	.host_server_socket = -1,
	.target_server_socket = -1,
	.target_count = 0,
	.config_flag = 0,
	.app_launch_timerfd = -1,
	.connect_timeout_timerfd = -1,
	.sampling_thread = -1,
	.replay_thread = -1,
	.transfer_thread = -1,
	.buf_fd = -1,
	.user_ev_fd = -1,
	.efd = -1,

	.host = {
		.control_socket = -1,
		.data_socket = -1,
		.data_socket_mutex = PTHREAD_MUTEX_INITIALIZER
	},

	.target = {
		{0L, },
	},
	.fd = {
		.brightness = -1,
		.voltage = -1,
		.procmeminfo = -1,
		.video = NULL,
		.audio_status = NULL,
		.procstat = NULL,
		.networkstat = NULL,
		.diskstats = NULL
	},
	.appPath = {0, }

	};
// =============================================================================
// util functions
// =============================================================================

static void write_int(FILE *fp, int code)
{
	fprintf(fp, "%d", code);
}

// =============================================================================
// atexit functions
// =============================================================================

void _close_server_socket(void)
{
	LOGI("close_server_socket\n");
	// close server socket
	if(manager.host_server_socket != -1)
		close(manager.host_server_socket);
	if(manager.target_server_socket != -1)
		close(manager.target_server_socket);
}

static void _unlink_files(void)
{
	LOGI("unlink files start\n");
	unlink(PORTFILE);
	unlink(SINGLETON_LOCKFILE);
	LOGI("unlink files done\n");
}

void unlink_portfile(void)
{
	unlink(PORTFILE);
}

// =============================================================================
// making sockets
// =============================================================================

// return 0 for normal case
static int makeTargetServerSocket()
{
	struct sockaddr_un serverAddrUn;

	if(manager.target_server_socket != -1)
		return -1;	// should be never happend

	// remove existed unix domain socket file
	unlink(UDS_NAME);

	if ((manager.target_server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		LOGE("Target server socket creation failed\n");
		return -1;
	}

#ifndef LOCALTEST
	// set smack attribute for certification
	fsetxattr(manager.target_server_socket, "security.SMACK64IPIN", "*", 1, 0);
	fsetxattr(manager.target_server_socket, "security.SMACK64IPOUT", "*", 1, 0);
#endif /* LOCALTEST */

	bzero(&serverAddrUn, sizeof(serverAddrUn));
	serverAddrUn.sun_family = AF_UNIX;
	sprintf(serverAddrUn.sun_path, "%s", UDS_NAME);

	if (-1 == bind(manager.target_server_socket, (struct sockaddr*) &serverAddrUn,
					sizeof(serverAddrUn)))
	{
		LOGE("Target server socket binding failed\n");
		return -1;
	}

	if(chmod(serverAddrUn.sun_path, 0777) < 0)
	{
		LOGE("Failed to change mode for socket file : errno(%d)\n", errno);
	}


	if (-1 == listen(manager.target_server_socket, 5))
	{
		LOGE("Target server socket listening failed\n");
		return -1;
	}

	LOGI("Created TargetSock %d\n", manager.target_server_socket);
	return 0;
}

// return port number for normal case
// return negative value for error case
static int makeHostServerSocket()
{
	struct sockaddr_in serverAddrIn;
	int opt = 1;
	int port;

	if(manager.host_server_socket != -1)
		return -1;	// should be never happened

	if ((manager.host_server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		LOGE("Host server socket creation failed\n");
		return -1;
	}

	if(setsockopt(manager.host_server_socket,
	   SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		LOGE("Failed to set socket option : errno(%d)\n", errno);
	}

	memset(&serverAddrIn, 0, sizeof(serverAddrIn));
	serverAddrIn.sin_family = AF_INET;
	serverAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind address to server socket
	for(port = INIT_PORT; port < LIMIT_PORT; port++)
	{
		serverAddrIn.sin_port = htons(port);
		if (0 == bind(manager.host_server_socket,
					(struct sockaddr*) &serverAddrIn, sizeof(serverAddrIn)))
			break;
	}

	if(port == LIMIT_PORT)
	{
		LOGE("Host server socket binding failed\n");
		return -1;
	}

	// enter listen state from client
	if (-1 == listen(manager.host_server_socket, 5))
	{
		LOGE("Host server socket listening failed\n");
		return -1;
	}

	LOGI("Created HostSock %d\n", manager.host_server_socket);
	return port;
}

// =============================================================================
// initializing / finalizing functions
// =============================================================================

static bool ensure_singleton(const char *lockfile)
{
	if (access(lockfile, F_OK) == 0)	/* File exists */
		return false;

	int lockfd = open(lockfile, O_WRONLY | O_CREAT, 0600);
	if (lockfd < 0) {
		LOGE("singleton lock file creation failed");
		return false;
	}
	/* To prevent race condition, also check for lock availiability. */
	bool locked = flock(lockfd, LOCK_EX | LOCK_NB) == 0;

	if (!locked) {
		LOGE("another instance of daemon is already running");
	}
	close(lockfd);
	return locked;
}

static void inititialize_manager_targets(__da_manager * mng)
{
	int index;
	__da_target_info target_init_value = {
		.pid = -1,
		.socket = -1,
		.event_fd = -1,
		.recv_thread = -1,
		.initial_log = 0,
		.allocmem = 0,
		.starttime = 0
	};

	for (index = 0; index < MAX_TARGET_COUNT; index++)
		mng->target[index] = target_init_value;

	manager.target_count = 0;
}

static bool initialize_pthread_sigmask()
{
	sigset_t newsigmask;

	sigemptyset(&newsigmask);
	sigaddset(&newsigmask, SIGALRM);
	sigaddset(&newsigmask, SIGUSR1);
	return pthread_sigmask(SIG_BLOCK, &newsigmask, NULL) == 0;
}

// return 0 for normal case
static int initializeManager(FILE *portfile)
{
	if (init_buf() != 0) {
		LOGE("Cannot init buffer\n");
		return -1;
	}

	if (initialize_system_info() < 0) {
		write_int(portfile, ERR_INITIALIZE_SYSTEM_INFO_FAILED);
		return -1;
	}
	// make server socket
	if (makeTargetServerSocket() != 0) {
		write_int(portfile, ERR_TARGET_SERVER_SOCKET_CREATE_FAILED);
		return -1;
	}
	if (!initialize_pthread_sigmask()) {
		write_int(portfile, ERR_SIGNAL_MASK_SETTING_FAILED);
		return -1;
	}

	int port = makeHostServerSocket();
	if (port < 0) {
		write_int(portfile, ERR_HOST_SERVER_SOCKET_CREATE_FAILED);
		return -1;
	} else {
		write_int(portfile, port);
	}

	LOGI("SUCCESS to write port\n");

	inititialize_manager_targets(&manager);

	// initialize sendMutex
	pthread_mutex_init(&(manager.host.data_socket_mutex), NULL);

	return 0;
}


static int finalizeManager()
{
	LOGI("Finalize daemon\n");
#ifndef LOCALTEST
	LOGI("finalize system info\n");
	finalize_system_info();
#endif

	// close host client socket
	if(manager.host.control_socket != -1){
		LOGI("close host control socket (%d)\n", manager.host.control_socket);
		close(manager.host.control_socket);
	}
	if(manager.host.data_socket != -1){
		LOGI("close host data socket (%d)\n", manager.host.data_socket);
		close(manager.host.data_socket);
	}

	LOGI("return\n");
	return 0;
}

static void remove_buf_modules(void)
{
	LOGI("rmmod buffer start\n");
	if (system("cd /opt/swap/sdk && ./stop.sh")) {
		LOGW("Cannot remove swap modules\n");
	}
	LOGI("rmmod buffer done\n");
}

static void terminate(int sig)
{
	_unlink_files();
	_close_server_socket();
	exit_buf();
	remove_buf_modules();
	if (sig != 0) {
		LOGW("Terminating due signal %s\n", strsignal(sig));
		signal(sig, SIG_DFL);
		raise(sig);
	}
}

static void terminate0()
{
	terminate(0);
}


static void setup_signals()
{
	struct sigaction sigact = {
		.sa_handler = terminate,
		.sa_flags = 0
	};
	sigemptyset(&sigact.sa_mask);
	sigaction(SIGTERM, &sigact, 0);
	sigaction(SIGINT, &sigact, 0);

	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE,SIG_IGN);
}

// main function
int main()
{
	if (!ensure_singleton(SINGLETON_LOCKFILE))
		return 1;

	initialize_log();

	LOGI("da_started\n");
	atexit(terminate0);


	//for terminal exit
	setup_signals();
	daemon(0, 1);
	LOGI("--- daemonized (pid %d) ---\n", getpid());

	FILE *portfile = fopen(PORTFILE, "w");
	if (!portfile) {
		LOGE("cannot create portfile");
		return 1;
	}

	int err = initializeManager(portfile);
	fclose(portfile);
	if (err)
		return 1;

	//init all file descriptors
	init_system_file_descriptors();
	//daemon work
	//FIX ME remove samplingThread it is only for debug
	//samplingThread(NULL);
	daemonLoop();
	LOGI("daemon loop finished\n");
	stop_all();
	finalizeManager();

	close_system_file_descriptors();

	LOGI("main finished\n");
	return 0;
}
