#ifndef __C_PROBE_H__
#define __C_PROBE_H__

#include "CProbeData.h"
#include "CListContainer.h"

#include "consts.h"
#include "debug.h"

/* FIXME remake it */
enum probe_t{
	PROBE_TYPE_SIMPLE = 0,
	PROBE_TYPE_FBI = 0x8
};

class CProbeListElm : public CPrintable {
	public:
		uint64_t m_Addr;
		probe_t m_Type;
		CProbeData *m_Data;

		virtual int addData(CProbeData *data) = 0;
		virtual int setData(CProbeData *data) = 0;

		CProbeListElm(uint64_t addr, probe_t ptype);
		virtual ~CProbeListElm();
};

typedef std::shared_ptr<CProbeListElm> pProbeListElm;
typedef std::list<pProbeListElm> TProbeList;
typedef TProbeList::iterator itProbeList;

class CProbeListContainer: public CListContainer<CProbeListElm>{
	public:
		virtual int elmCmp(pProbeListElm a, pProbeListElm b);
		CProbeListElm *setCurOrCreate(uint64_t addr, probe_t type);
};

#endif /* __C_PROBE_H__ */
