#ifndef __IINSTRUMENTATION_H__
#define __IINSTRUMENTATION_H__

#include "IInterface.h"
#include "debug.h"
#if defined (STANDALONE) || !defined (TARGET)
#include "CConfig.h"
#endif

typedef enum {
	BSECI_STATE = 0,
	BSECI_MODE,
	BSECI_BUFFER_SIZE,
	BSECI_BUFFER_EFFECT,
	BSECI_TRACE_SIZE,
	BSECI_FIRST,
	BSECI_AFTER_LAST,
	BSECI_IGNORED,
	BSECI_SAVED,
	BSECI_DISCARDED,
	BSECI_COLLISION,
	BSECI_LOST,
	BSECI_COUNT,
} TYPE_BSECI;

class IInstrumentation //: public CError
{
public:
	IInstrumentation() { };
	virtual ~IInstrumentation() { };

public:
	virtual int GetBufferStatus(unsigned long* pnArray, unsigned int nSize) = 0;
#if defined (STANDALONE) || !defined (TARGET)
	virtual int SetConfig(const char *szConfig, CConfig &profile) = 0;
#endif
	virtual int StartTrace() = 0;
	virtual int StopTrace() = 0;
	virtual int ReadTrace() = 0;
	virtual char *GetClientBuffer() = 0;
	virtual int GetClientBufferSize() = 0;
	virtual int SetClientBuffer(char * pBuffer, unsigned long nSize) = 0;
	virtual int SetClientBufferSize(unsigned long nSize) = 0;
};

#endif // __IINSTRUMENTATION_H__
