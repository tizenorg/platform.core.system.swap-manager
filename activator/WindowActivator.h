/*
 * Window Activator
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim <jaewon91.lim@samsung.com>
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
 * @summary          :
 * @create date      : 2011. 8. 16.
 * @author(create)   : Lee JaeYeol / jaeyeol148.lee@samsung.com / S-Core
 * @revision date    : 2011. 8. 16.
 * @author(revision) : Lee JaeYeol / jaeyeol148.lee@samsung.com / S-Core
 */


#ifndef WINDOWACTIVATOR_H_
#define WINDOWACTIVATOR_H_

#include <string>

class WindowActivator
{
private:
    WindowActivator();
	virtual ~WindowActivator();

public:
    static bool ActivateWindow(const std::string pid);
    static bool ActivateWindow(int pid);
};

#endif /* WINDOWACTIVATOR_H_ */
