#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <stdbool.h>
#include <errno.h>
#include "elf.h"

#define SIZEOF_VOID_P 4
#if SIZEOF_VOID_P == 8
typedef Elf64_Ehdr Elf_Ehdr;
#elif SIZEOF_VOID_P == 4
typedef Elf32_Ehdr Elf_Ehdr;
#else
#error "Unknown void* size"
#endif

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
