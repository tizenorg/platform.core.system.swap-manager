#ifndef __C_PROBE_H__
#define __C_PROBE_H__

#include "CVisitor.h"

#include "CProbeData.h"
#include "CListContainer.h"

#include "consts.h"
#include "debug.h"

/* FIXME remake it */
enum probe_t{
	PROBE_TYPE_SIMPLE = 0,
	PROBE_TYPE_FBI = 0x8
};

class CProbeListElm : public CNode {
	public:
		uint64_t m_Addr;
		probe_t m_Type;
		CProbeData *m_Data;

		void accept(CVisitor& v); /* CNode */

		virtual int addData(CProbeData *data) = 0;
		virtual int setData(CProbeData *data) = 0;
		int printElm(){};
		int printList(){};



		CProbeListElm(uint64_t addr, probe_t ptype);
		virtual ~CProbeListElm();
};

typedef std::shared_ptr<CProbeListElm> pProbeListElm;
typedef std::list<pProbeListElm> TProbeList;
typedef TProbeList::iterator itProbeList;

class CProbeListContainer: public CListContainer<CProbeListElm>{
	public:
		virtual void accept(CVisitor& v);
		virtual int elmCmp(pProbeListElm a, pProbeListElm b);
		CProbeListElm *setCurOrCreate(uint64_t addr, probe_t type);
};

#endif /* __C_PROBE_H__ */
