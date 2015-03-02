
#include "CProbeDataFBIVar.h"
#include "CProbeFBI.h"
#include "debug.h"


int CProbeFBI::addData(CProbeData *data) {
	TRACE("> %p", data);
	data->printElm();
	m_Data->addData(data);
}

int CProbeFBI::setData(CProbeData *data) {
	TRACE("> %p", data);

	/* TODO is it necessary? */
	if (m_Data != NULL)
		delete m_Data;

	m_Data = data;
}

void CProbeFBI::printElm()
{
	TRACE("probe '%lx', %d", m_Addr, m_Type);
}

void CProbeFBI::printList()
{
	printElm();
	TRACE(">>>LIST:");
	if (m_Data != NULL)
		m_Data->printList();
	else
		LOGE("m_data = NULL");
}

CProbeFBI::CProbeFBI(uint64_t addr, probe_t ptype)
:CProbeListElm(addr, ptype)
{
	/* Create Variables list */
	m_Data = pCProbeDataFBIVarList(new CProbeDataFBIVarList());
}

CProbeFBI::~CProbeFBI() {
	TRACE("Delete FBI probe %d %lx", m_Type, m_Addr);
	delete m_Data;
}

