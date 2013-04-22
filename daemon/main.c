/*
*  DA manager
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact:
*
* Jaewon Lim <jaewon81.lim@samsung.com>
* Woojin Jung <woojin2.jung@samsung.com>
* Juyoung Kim <j0.kim@samsung.com>
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
*
*/

#include <stdio.h>			// for fopen, fprintf
#include <stdlib.h>			// for atexit
#include <sys/types.h>		// for open
#include <sys/stat.h>		// for open
#include <sys/socket.h>		// for socket
#include <sys/un.h>			// for sockaddr_un
#include <arpa/inet.h>		// for sockaddr_in, socklen_t

#include <signal.h>			// for signal
#include <unistd.h>			// for unlink
#include <fcntl.h>			// for open, fcntl
#include <attr/xattr.h>		// for fsetxattr

#include "daemon.h"
#include "sys_stat.h"

#define SINGLETON_LOCKFILE			"/tmp/lockfile.da"
#define PORTFILE					"/tmp/port.da"
#define UDS_NAME					"/tmp/da.socket"

#define INIT_PORT				8001
#define LIMIT_PORT				8100

// initialize global variable
__da_manager manager =
{
	NULL, -1, -1,								// portfile pointer, host / target server socket
	0,										// target count
	0,										// config_flag
	-1,										// sampling_thread handle
	{-1, -1, PTHREAD_MUTEX_INITIALIZER},	// host
	{{0L, }, },								// target
	{0, }									// appPath
};

// ==============================================================================
// util functions
// ==============================================================================

static void writeToPortfile(int code)
{
	if(unlikely(manager.portfile == 0))
		return;

	fprintf(manager.portfile, "%d", code);
}

// ==============================================================================
// atexit functions
// ==============================================================================

void _close_server_socket(void)
{
	LOGI("close_server_socket\n");
	// close server socket
	if(manager.host_server_socket != -1)
		close(manager.host_server_socket);
	if(manager.target_server_socket != -1)
		close(manager.target_server_socket);
}

static void _unlink_file(void)
{
	LOGI("unlink files\n");
	unlink(PORTFILE);
	unlink(SINGLETON_LOCKFILE);
}

void unlink_portfile(void)
{
	unlink(PORTFILE);
}

// ===============================================================================
// making sockets
// ===============================================================================

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

	// set smack attribute for certification
	fsetxattr(manager.target_server_socket, "security.SMACK64IPIN", "*", 1, 0);
	fsetxattr(manager.target_server_socket, "security.SMACK64IPOUT", "*", 1, 0);

	bzero(&serverAddrUn, sizeof(serverAddrUn));
	serverAddrUn.sun_family = AF_UNIX;
	sprintf(serverAddrUn.sun_path, "%s", UDS_NAME);

	if (-1 == bind(manager.target_server_socket, (struct sockaddr*) &serverAddrUn,
					sizeof(serverAddrUn)))
	{
		LOGE("Target server socket binding failed\n");
		return -1;
	}

	chmod(serverAddrUn.sun_path, 0777);

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

	setsockopt(manager.host_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	memset(&serverAddrIn, 0, sizeof(serverAddrIn));
	serverAddrIn.sin_family = AF_INET;
	serverAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind address to server socket
	for(port = INIT_PORT; port < LIMIT_PORT; port++)
	{
		serverAddrIn.sin_port = htons(port);
		if (0 == bind(manager.host_server_socket, (struct sockaddr*) &serverAddrIn, sizeof(serverAddrIn)))
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

// ===============================================================================
// initializing / finalizing functions
// ===============================================================================

static int singleton(void)
{
	struct flock fl;
	int fd;

	fd = open(SINGLETON_LOCKFILE, O_RDWR | O_CREAT, 0600);
	if(fd < 0)
	{
		writeToPortfile(ERR_LOCKFILE_CREATE_FAILED);
		LOGE("singleton lock file creation failed");
		return -1;
	}

	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	if(fcntl(fd, F_SETLK, &fl) < 0)
	{
		writeToPortfile(ERR_ALREADY_RUNNING);
		LOGE("another instance of daemon is already running");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

// return 0 for normal case
static int initializeManager()
{
	int i;

	atexit(_close_server_socket);

	if(initialize_system_info() < 0)
	{
		writeToPortfile(ERR_INITIALIZE_SYSTEM_INFO_FAILED);
		return -1;
	}

	// make server socket
	if(makeTargetServerSocket() != 0)
	{
		writeToPortfile(ERR_TARGET_SERVER_SOCKET_CREATE_FAILED);
		return -1;
	}

	i = makeHostServerSocket();
	if(i < 0)
	{
		writeToPortfile(ERR_HOST_SERVER_SOCKET_CREATE_FAILED);
		return -1;
	}
	else	// host server socket creation succeed
	{
		writeToPortfile(i);
	}

	LOGI("SUCCESS to write port\n");

	// initialize target client sockets
	for (i = 0; i < MAX_TARGET_COUNT; i++)
	{
		manager.target[i].pid = -1;
		manager.target[i].socket = -1;
		manager.target[i].event_fd = -1;
		manager.target[i].recv_thread = -1;
		manager.target[i].initial_log = 0;
		manager.target[i].allocmem = 0;
		manager.target[i].starttime = 0;
	}
	manager.target_count = 0;

	// initialize sendMutex
	pthread_mutex_init(&(manager.host.data_socket_mutex), NULL);

	return 0;
}

static int finalizeManager()
{
	finalize_system_info();

	LOGI("Finalize daemon\n");

	// close host client socket
	if(manager.host.control_socket != -1)
		close(manager.host.control_socket);
	if(manager.host.data_socket != -1)
		close(manager.host.data_socket);

	return 0;
}

// main function
int main()
{
	int result;
	initialize_log();

	atexit(_unlink_file);

	manager.portfile = fopen(PORTFILE, "w");
	if(manager.portfile == NULL)
	{
		LOGE("cannot create portfile");
		return 1;
	}

	if(singleton() < 0)
		return 1;

	//for terminal exit
	signal(SIGHUP, SIG_IGN);
	chdir("/");

	//new session reader
	setsid();

	// initialize manager
	result = initializeManager();
	fclose(manager.portfile);
	if(result == 0)
	{
		//daemon work
		daemonLoop();

		finalizeManager();
		return 0;
	}
	else
	{
		return 1;
	}
}
