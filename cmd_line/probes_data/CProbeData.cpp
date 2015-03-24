
#include "CProbeData.h"
#include "CVisitor.h"

/* TODO remove this function and make it "virtual = 0" */

void CProbeData::accept(CVisitor& v)
{
	LOGE("This function could not be called");
}


CProbeData::CProbeData(datatype_t data_type)
:m_DataType(data_type)
{
}

CProbeData::~CProbeData()
{
}
