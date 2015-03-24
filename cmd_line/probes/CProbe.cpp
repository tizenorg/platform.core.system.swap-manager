#include "CProbeData.h"
#include "CProbeFBI.h"
#include "debug.h"

/* CProbeListElm */
void CProbeListElm::accept(CVisitor& v) /* CNode */
{
	if (v.access(this) == CVisitor::ACCESS_GRANTED) {
		v.entry(this);
		m_Data->accept(v);
		v.exit(this);
	}
}

CProbeListElm::CProbeListElm(uint64_t addr, probe_t ptype)
{
	m_Addr = addr;
	m_Type = ptype;
	m_Data = NULL;
}

CProbeListElm::~CProbeListElm()
{
}

/* CProbeListContainer */
void CProbeListContainer::accept(CVisitor& v) /* CNode */
{
	itList i;
	if (v.access(this) == CVisitor::ACCESS_GRANTED) {
		v.entry(this);
		for (i = m_List->begin(); i != m_List->end(); i++) {
			i->get()->accept(v);
		}
		v.exit(this);
	}
}

int CProbeListContainer::elmCmp(pProbeListElm a, pProbeListElm b) {
	int res;

	res = a.get()->m_Addr != b.get()->m_Addr ||
	      a.get()->m_Type != b.get()->m_Type;

	return res;
}

CProbeListElm *CProbeListContainer::setCurOrCreate(uint64_t addr, probe_t type)
{
	itProbeList probe;
	TRACE("Probe: addr ='%lx', type=%d", addr, type);

	pProbeListElm new_probe;
	/* TODO refactor this creator */
	switch (type) {
		case PROBE_TYPE_FBI:
			new_probe = pProbeListElm(new CProbeFBI(addr, type));
			break;
		default:
			LOGE("Wrong probe type: " << type);
			return NULL;
	}
	return _setCurOrCreate(new_probe);
}

