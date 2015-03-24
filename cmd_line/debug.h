#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <memory>
#include <list>

#include "stddebug.h"

#ifdef __cplusplus

#include <stdio.h>
#include <iostream>
#include <cerrno>

#define LOGI(...) std::cout << "[INF] "  << typeid(this).name() << "::" <<  __FUNCTION__  << "[" << __LINE__ << "]:" << __VA_ARGS__ << std::endl
#define LOGE(...) std::cout << "[ERR] "  << __FUNCTION__  << "[" << __LINE__ << "]:" << __VA_ARGS__ << std::endl

#else /* __cplusplus */

#define LOGE(format, arg...) 									\
do { 												\
	printf("ERR[%s]: %s: %u: " format "\n", __FILE__, __FUNCTION__, __LINE__, ## arg ); 	\
} while(0)

#define LOGI(format, arg...) 									\
do { 												\
	printf("INF[%s]: %s: %u: " format "\n", __FILE__, __FUNCTION__, __LINE__, ## arg ); 	\
} while(0)

#endif /* __cplusplus */

#endif /* __DEBUG_H__ */
