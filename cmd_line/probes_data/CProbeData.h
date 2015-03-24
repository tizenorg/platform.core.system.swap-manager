#ifndef __CPROBEDATA_H__
#define __CPROBEDATA_H__

#include "CVisitor.h"
#include "debug.h"



enum datatype_t {
	PROBE_DATA_FBI_VAR,
	PROBE_DATA_FBI_STEP,
	PROBE_DATA_FBI_VAR_LIST,
	PROBE_DATA_FBI_STEP_LIST,
};

class CProbeData : public CNode {
	public:
		typedef std::shared_ptr<CProbeData> pCProbeData;

		virtual int setData(CProbeData *data) = 0;
		virtual int addData(CProbeData *data) = 0;

		datatype_t m_DataType;

		virtual void accept(CVisitor& v); /* CNode */
		int printElm(){};
		int printList(){};


		CProbeData(datatype_t data_type);

		virtual ~CProbeData();
};
typedef std::shared_ptr<CProbeData> pCProbeData;

#endif /* __CPROBEDATA_H__ */
