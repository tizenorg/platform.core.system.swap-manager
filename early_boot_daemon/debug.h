#ifndef __SWAP_DEBUG
#define __SWAP_DEBUG

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#define DEBUG
#ifdef DEBUG

#define log_errmsg(...) do_log("ERR", __func__, __LINE__, __VA_ARGS__)

#define log_debmsg(...) do_log("DEB", __func__, __LINE__, __VA_ARGS__)

#define errno_exit(msg, err)                                            \
        do {                                                            \
                char buf[256];                                          \
                strerror_r(err, buf, sizeof(buf));                      \
                do_log("ERR", __func__, __LINE__, "%s: %s\n", msg, buf);\
                exit (EXIT_FAILURE);                                    \
        } while (0)

#define log_errno(msg, err)                                             \
        do {                                                            \
                char buf[256];                                          \
                strerror_r(err, buf, sizeof(buf));                      \
                do_log("ERR", __func__, __LINE__, "%s: %s\n", msg, buf);\
        } while (0)

static inline void do_log(const char *prefix, const char *funcname, int line, ...) {
        va_list ap;
        const char *fmt;
        fprintf(stderr, "[%s] (%s:%d):", prefix, funcname, line);

        va_start(ap, line);
        fmt = va_arg(ap, const char *);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
}

#else
#define log_errmsg(...)
#define err_exit(msg, err)
#define log_err(msg, err)

#endif

#endif //__SWAP_DEBUG
