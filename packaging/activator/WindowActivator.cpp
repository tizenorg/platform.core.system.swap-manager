/*
 * Window Activator
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim <jaewon81.lim@samsung.com>
 * Juyoung Kim <j0.kim@samsung.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */



/**
 * @summary
 * @create date			: 2011. 8. 16.
 * @author(create)		: Lee JaeYeol / jaeyeol148.lee@samsung.com / S-Core
 * @revision date		: 2011. 8. 16.
 * @author(revision) 	: Lee JaeYeol / jaeyeol148.lee@samsung.com / S-Core
 * @refactoring date	: 2012. 4. 14
 * @author(refactoring) : Jaewon Lim / jaewon81.lim@samsung.com / S-Core
 */

#include "WindowActivator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

WindowActivator::WindowActivator()
{
}

WindowActivator::~WindowActivator()
{
}

bool WindowActivator::ActivateWindow(const std::string pid)
{
	return ActivateWindow(atoi(pid.c_str()));
}


#ifdef _WIN32 || _WIN64
#include <windows.h>
//#using <System.dll>
//using namespace System;
//using namespace System::Diagnostics;
//using namespace System::ComponentModel;

ULONG ProcIDFromWnd(HWND hwnd);
HWND GetWinHandle(ULONG pid);
void ActiveWindow(HWND hWnd);

bool WindowActivator::ActivateWindow(int pid)
{
    ActiveWindow(GetWinHandle(pid));
    return true;
}

ULONG ProcIDFromWnd(HWND hwnd)
{
    ULONG idProc;
    GetWindowThreadProcessId( hwnd, &idProc );
    return idProc;
}

HWND GetWinHandle(ULONG pid)
{
//	Process^ fproc = Process::GetProcessById(pid);
//	return (HWND)fproc->MainWindowHandle.ToPointer();

    HWND tempHwnd = FindWindow(NULL,NULL);

    while(tempHwnd != NULL )
    {
        if(GetParent(tempHwnd) == NULL )
		{
			if(GetWindow(tempHwnd, GW_OWNER) == NULL)
			{
	            if( pid == ProcIDFromWnd(tempHwnd) )
		            return tempHwnd;
			}
		}
        tempHwnd = GetWindow(tempHwnd, GW_HWNDNEXT);
    }
    return NULL;
}

void ActiveWindow(HWND hWnd)
{
    HWND hWndForeground = ::GetForegroundWindow();
    if(hWndForeground == hWnd)
	{
		printf("same as foreground window\n");
		return;
	}

    DWORD Strange = ::GetWindowThreadProcessId(hWndForeground, NULL);
    DWORD My = ::GetWindowThreadProcessId(hWnd, NULL);
    if( !::AttachThreadInput(Strange, My, TRUE) )
    {
        //      ASSERT(0);
    }
    ::SetForegroundWindow(hWnd);
    ::BringWindowToTop(hWnd);
    if( !::AttachThreadInput(Strange, My, FALSE) )
    {
        //     ASSERT(0);
    }
    if (::IsIconic(hWnd))
        ::ShowWindow(hWnd, SW_RESTORE);
}

#elif __linux	// if Linux

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#define PERROR(...)	fprintf(stderr, __VA_ARGS__)

bool findWindow(Display *disp, int idepid);
Window* getClientList(Display *disp, unsigned long *size);
char *getProperty(Display *disp, Window win, Atom xaPropType, const char *propName, unsigned long *size);
char *getWindowTitle(Display *disp, Window win);
int findProcessID(Display *disp, Window win);
void activateWindow(Display *disp, Window win);
bool sendEvent(Display *disp, Window win, char *msg, unsigned long data0, unsigned long data1, unsigned long data2, unsigned long data3, unsigned long data4);

bool WindowActivator::ActivateWindow(int pid)
{

	Display *disp;

   	if (!(disp = XOpenDisplay(NULL)))
   	{
    		PERROR("XOpenDisplay error");
        	return false;
    	}

    	findWindow(disp, pid);
    	return true;
}

bool findWindow(Display *disp, int idepid)
{
	unsigned long clientListSize;
	unsigned long i;
	Window *clientList;
	Window activate = 0;

	if ((clientList = getClientList(disp, &clientListSize)) == NULL)
	{
		return false;
	}

	for (i = 0; i < clientListSize / sizeof(Window); i++)
	{
		int pid = 0;
		pid = findProcessID(disp, clientList[i]);

		if (pid == 0)
		{
			continue;
		}

		if(pid == idepid)
		{
			activate = clientList[i];
			break;
		}
	}

	free(clientList);

	if (activate)
	{
		activateWindow(disp, activate);
		return true;
	}
	else
	{
		return false;
	}
}

Window* getClientList(Display *disp, unsigned long *size)
{
	Window *clientList;

	if ((clientList = (Window *)getProperty(disp, DefaultRootWindow(disp), XA_WINDOW, (char*)"_NET_CLIENT_LIST", size)) == NULL)
	{
		if ((clientList = (Window *)getProperty(disp, DefaultRootWindow(disp), XA_CARDINAL, (char*)"_WIN_CLIENT_LIST", size)) == NULL)
		{
			PERROR("Can not get a client list");
			return NULL;
		}
	}
	return clientList;
}

char* getProperty(Display *disp, Window win, Atom xaPropType, const char *propName, unsigned long *size)
{
	Atom xaPropName;
	Atom xaRetType;
	int retFormat;
	unsigned long retItems;
	unsigned long retBytesAfter;
	unsigned long tmpSize;
	unsigned char *retProp;
	char *ret;

	xaPropName = XInternAtom(disp, propName, False);

	int returnValue = XGetWindowProperty(disp, win, xaPropName, 0, 1024, False, xaPropType, &xaRetType, &retFormat, &retItems, &retBytesAfter, &retProp);
	PERROR("XGetWindowProperty  %d ", returnValue);

	if (returnValue != Success)
	{
		PERROR("Can not get a %s property", propName);
		return NULL;
	}

    if (xaRetType != xaPropType)
    {
    	PERROR("Invalid type of %s property.", propName);
    	XFree(retProp);
    	return NULL;
    }

    // null terminate the result to make string handling easier /
    tmpSize = (retFormat / 8) * retItems;
    // Correct 64 Architecture implementation of 32 bit data /
    if(retFormat==32) tmpSize *= sizeof(long)/4;
    ret = (char*)malloc(tmpSize + 1);
    memcpy(ret, retProp, tmpSize);
    ret[tmpSize] = '\0';

    if (size) {
        *size = tmpSize;
    }

    XFree(retProp);
    return ret;
}

int findProcessID(Display *disp, Window win)
{
	int pid = 0;
	unsigned long *netWmPid;

	netWmPid = (unsigned long*)getProperty(disp, win, XA_CARDINAL, (const char*)"_NET_WM_PID", NULL);

	if(netWmPid)
	{
		pid = (int)*netWmPid;
		free(netWmPid);
	}

	PERROR("findProcessID  %d ", pid);
	return pid;
}

bool sendEvent(Display *disp, Window win, char *msg,
		unsigned long data0, unsigned long data1,
        unsigned long data2, unsigned long data3,
        unsigned long data4)
{
    XEvent event;
    long mask = SubstructureRedirectMask | SubstructureNotifyMask;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom(disp, msg, False);
    event.xclient.window = win;
    event.xclient.format = 32;
    event.xclient.data.l[0] = data0;
    event.xclient.data.l[1] = data1;
    event.xclient.data.l[2] = data2;
    event.xclient.data.l[3] = data3;
    event.xclient.data.l[4] = data4;

    if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event))
    {
        return true;
    }
    else
    {
    	PERROR("Can not send %s event", msg);
        return false;
    }
}


void activateWindow (Display *disp, Window win)
{
    unsigned long *desktop = NULL;

    // desktop ID /
    if ((desktop = (unsigned long *)getProperty(disp, win, XA_CARDINAL, (const char*)"_NET_WM_DESKTOP", NULL)) == NULL)
    {
        if ((desktop = (unsigned long *)getProperty(disp, win, XA_CARDINAL, (const char*)"_WIN_WORKSPACE", NULL)) == NULL)
        {
        	PERROR("Can not find desktop ID");
        }
    }

    if (desktop)
    {
        if (sendEvent(disp, DefaultRootWindow(disp), (char*)"_NET_CURRENT_DESKTOP", *desktop, 0, 0, 0, 0) != EXIT_SUCCESS)
        {
        	PERROR("Can not switch desktop.");
        }
        free(desktop);
    }

    sendEvent(disp, win, (char*)"_NET_ACTIVE_WINDOW", 0, 0, 0, 0, 0);
    XMapRaised(disp, win);

    XFlush(disp);
    XCloseDisplay (disp);
}
#elif __APPLE__

#include <getopt.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <ApplicationServices/ApplicationServices.h>

bool WindowActivator::ActivateWindow(int pid)
{

        ProcessSerialNumber psn;
        if(noErr == GetProcessForPID(pid, &psn))
        {
                SetFrontProcess(&psn);
                exit(0);
        }
        return 0;
}

#endif	// end Linux
