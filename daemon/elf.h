#ifndef _ELF_
#define _ELF_

enum binary_type_t {
	BINARY_TYPE_NO_PIE = 0,
	BINARY_TYPE_PIE = 1,
	BINARY_TYPE_UNKNOWN
};

uint32_t get_binary_type(const char *path);

#endif /* _ELF_ */
