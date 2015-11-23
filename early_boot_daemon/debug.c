#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "debug.h"

#define DEBUG_LOGFILE   "/swap_readerd.log"

#ifdef DEBUG
static inline int close_on_exec_dup(int old, int new) {
        int ret = -1;

        if (dup2(old, new) != -1) {
                unsigned long flags = fcntl(new, F_GETFD);
                if (flags == -1) {
                        log_errmsg ("can not get flags fd #%d errno <%d>\n", new,
                             errno);
                        goto err_ret;
                }

                if (fcntl(new, F_SETFD, flags | FD_CLOEXEC) == -1) {
                        log_errmsg("can not set flags fd #%d errno <%d>\n", new,
                             errno);
                        goto err_ret;
                }
        } else {
                log_errmsg("dup2 failed\n");
                goto err_ret;
        }

        /* success */
        ret = 0;

err_ret:
        return ret;
}

int initialize_log(void) {
        int ret = 0;
        int fd = -1;
        int fd_null = -1;

/*
        if (remove(DEBUG_LOGFILE))
                log_errmsg("remove(%s), return error, errno=%d\n",
                     DEBUG_LOGFILE, errno);
*/
        fd = open(DEBUG_LOGFILE, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        fd_null = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0777);

        if (fd != -1 && fd_null != -1) {
                if (close_on_exec_dup(fd_null, 1) != 0 ||
                    close_on_exec_dup(fd, 2) != 0) {
                        log_errmsg("duplicate fd fail\n");
                        ret = -1;
                }
        } else {
                close(1);
                close(2);
        }

        if (fd_null != -1)
                close(fd_null);

        if (fd != -1)
                close(fd);

        close(0);
        return ret;
}
#else
int initialize_log(void) {
        return 0;
}
#endif
