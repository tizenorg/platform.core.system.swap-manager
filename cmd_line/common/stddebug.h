#ifndef __STDDEBUG_H__
#define __STDDEBUG_H__

#include <stdio.h>
#include <sys/time.h>

#define FILE_NAME "./swaplog.txt"

extern char *logFile;

#ifdef __cplusplus
class CException
{
	public :
		CException() { }
};
#endif

#if defined(__DEBUG)
	#if defined(__ENABLE_LOG)
		#define TRACE(format, arg...) 											\
		{														\
			FILE * pFileLog = fopen(logFile, "a");									\
			if(pFileLog != NULL) {											\
				fprintf(pFileLog, "TRACE[%s:%u] %s " format "\n", __FILE__, __LINE__, __FUNCTION__, ## arg ); 	\
				fflush(pFileLog); 										\
				fclose(pFileLog);										\
			} 													\
                }
	#else
		#define TRACE(format, arg...) 									\
		{ 												\
			printf("TRACE[%s]: %s: %u: " format "\n", __FILE__, __FUNCTION__, __LINE__, ## arg ); 	\
                }
	#endif // __ENABLE_LOG

#if defined(__ANDROID)
#define ASSERT(x)
#else
#define ASSERT(x) if((x) == false) { TRACE("ASSERT(%s) in [%s:%u] %s \n", #x, __FILE__, __LINE__, __FUNCTION__); throw new CException(); }
#endif
	#define ENSURE(x) if((x) == false) TRACE("ENSURE(%s) in [%s:%u] %s \n", #x, __FILE__, __LINE__, __FUNCTION__);
#else
	#define TRACE(format, arg...)
	#define ASSERT(x)
	#define ENSURE(x)
#endif /* __DEBUG */

#endif /* __STDDEBUG_H__ */
