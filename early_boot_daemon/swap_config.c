#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"

struct basic_msg_fmt {
        uint32_t msg_id;                         /**< Message ID. */
        uint32_t len;                            /**< Message length. */
} __packed;


int swap_send_conf (int buf_fd, int conf_fd) {
	off_t conf_size;
	struct stat st;
	char *conf, *curr_conf_pos;
	struct basic_msg_fmt bmf;

	if (fstat(conf_fd, &st) == -1) {
		log_errno("Cannnot fstat config", errno);
		return -1;
	}
	conf_size = st.st_size;
	if (conf_size == 0) {
		log_errmsg("Offline config is empty\n");
		return -1;
	}

	if ((conf = malloc(conf_size)) == NULL) {
		log_errno("Can't allocate memory", errno);
		return -1;
	}

	if (read(conf_fd, conf, conf_size) != conf_size) {
		log_errno("Failed to read config file", errno);
		return -1;
	}

	curr_conf_pos = conf;
	while (curr_conf_pos < conf + conf_size) {
		memcpy(&bmf, curr_conf_pos, sizeof(bmf));
		if (bmf.msg_id != 0x0003) {
			if (ioctl_send_msg(buf_fd, curr_conf_pos) == -1)
				return -1;
			log_debmsg("Send conf %u\n", sizeof(bmf) + bmf.len);
		}
		curr_conf_pos += sizeof(bmf) + bmf.len;
	}
	return 0;
}
