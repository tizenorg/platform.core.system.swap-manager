#ifndef __C_PROBE_FBI_H__
#define __C_PROBE_FBI_H__

#include "CProbe.h"

class CProbeFBI : public CProbeListElm
{
	public:
		virtual int addData(CProbeData *data);
		virtual int setData(CProbeData *data);

		virtual void printElm();
		virtual void printList();

		CProbeFBI(uint64_t addr, probe_t ptype);

		virtual ~CProbeFBI();

};
#endif /* __C_PROBE_FBI_H__ */
