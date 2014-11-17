
#ifndef WSI_H_
#define WSI_H_

int wsi_init(const char *address, int port);
int wsi_destroy(void);
int wsi_start_profiling(void);
int wsi_stop_profiling(void);

#endif /* WSI_H_ */
