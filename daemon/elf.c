/*
 *  DA manager
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Cherepanov Vitaliy <v.cherepanov@samsung.com>
 * Dmitry Bogatov     <d.bogatov@samsung.com>
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
 * - Samsung RnD Institute Russia
 *
 */


#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "elf.h"

#define SIZEOF_VOID_P 4
#if SIZEOF_VOID_P == 8
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Shdr Elf_Shdr;
#elif SIZEOF_VOID_P == 4
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
#else
#error "Unknown void* size"
#endif

static size_t fsize(int fd)
{
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

static void *mmap_file(const char *filepath, size_t * len)
{
	int fd = open(filepath, O_RDONLY);
	if (fd < 0)
		return NULL;
	*len = fsize(fd);
	void *mem = mmap(NULL, *len, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	return mem == MAP_FAILED ? NULL : mem;
}

static const Elf_Shdr *elf_find_debug_header(const void *obj)
{
	const Elf_Ehdr *elf_header = obj;
	const Elf_Shdr *section_table = obj + elf_header->e_shoff;
	const Elf_Shdr *string_entry = section_table + elf_header->e_shstrndx;
	const char *string_section = obj + string_entry->sh_offset;
	int index;

	for (index = 0; index != elf_header->e_shnum; ++index) {
		const Elf_Shdr *entry = section_table + index;
		if (!strcmp(".debug_str", string_section + entry->sh_name))
			return entry;
	}
	return NULL;
}

static int read_elf_header(Elf_Ehdr * header, const char *filepath)
{
	int fd = open(filepath, O_RDONLY);
	if (fd < 0)
		return -ENOENT;
	bool read_failed = read(fd, header, sizeof(*header)) != sizeof(*header);
	close(fd);
	return read_failed ? -EIO : 0;
}

static int elf_type(const char *filepath)
{
	Elf_Ehdr header;
	int err = read_elf_header(&header, filepath);
	return err ? err : header.e_type;
}

uint32_t get_binary_type(const char *path)
{
	int type = elf_type(path);

	switch (type) {
	case ET_DYN:
		return BINARY_TYPE_PIE;
	case ET_EXEC:
		return BINARY_TYPE_NO_PIE;
	default:
		return BINARY_TYPE_UNKNOWN;
	}
}

static int is_like_absolute_path(const char *str)
{
	if (*str == '/')
		return 1;
	if (isupper(*str) && str[1] == ':' && str[2] == '\\')
		return 1;
	return 0;
}

void get_build_dir(char builddir[PATH_MAX], const char *filename)
{
	size_t len;
	void *filemem = mmap_file(filename, &len);
	if (filemem) {
		const Elf_Shdr *debug_header = elf_find_debug_header(filemem);
		if (debug_header) {
			const char *debug_section =
			    filemem + debug_header->sh_offset;
			const char *debug_section_end =
			    debug_section + debug_header->sh_size;
			const char *p = debug_section;
			/* `is_like_absolute_path' checks three chars forward. */
			while (p < debug_section_end - 3) {
				if (is_like_absolute_path(p)) {
					snprintf(builddir, PATH_MAX, "%s", p);
					return;
				}
				p = 1 + memchr(p, '\0', debug_section_end - p);
			}
		}
		munmap(filemem, len);
	}
	*builddir = '\0';
}
