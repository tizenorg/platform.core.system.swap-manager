/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim		<jaewon81.lim@samsung.com>
 * Woojin Jung		<woojin2.jung@samsung.com>
 * Juyoung Kim		<j0.kim@samsung.com>
 * Nikita Kalyazin	<n.kalyazin@samsung.com>
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


#include <stdlib.h>
#include <sys/smack.h>
#include <attr/xattr.h>
#include "smack.h"

#define SELF_LABEL_FILE "/proc/self/attr/current"
#define SMACK_LABEL_LEN 255

void fd_setup_attributes(int fd)
{
	fsetxattr(fd, "security.SMACK64IPIN", "*", 1, 0);
	fsetxattr(fd, "security.SMACK64IPOUT", "@", 1, 0);
}

void set_label_for_all(const char *path)
{
	smack_lsetlabel(path, "*", SMACK_LABEL_ACCESS);
}

int apply_smack_rules(const char* subject, const char* object,
		    const char* access_type)
{
	struct smack_accesses *rules = NULL;
	int ret = 0;

	if (smack_accesses_new(&rules))
		return -1;

	if (smack_accesses_add(rules, subject, object, access_type)) {
		smack_accesses_free(rules);
		return -1;
	}

	ret = smack_accesses_apply(rules);
	smack_accesses_free(rules);

	return ret;
}
