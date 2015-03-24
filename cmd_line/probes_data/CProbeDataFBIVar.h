#ifndef __CPROBE_DATA_FBI_VAR_H__
#define __CPROBE_DATA_FBI_VAR_H__

#include "CProbeDataList.h"
#include "CProbeData.h"

class FBIStep {
	public:
		uint8_t m_PointerOrder;
		uint64_t m_Offset;

		FBIStep(uint8_t PointerOrder, uint64_t Offset)
		{
			m_PointerOrder = PointerOrder;
			m_Offset = Offset;
			TRACE("->Create po=%d; off=0x%lx", PointerOrder, Offset);
		}
};

typedef std::shared_ptr<FBIStep> FBIStepElm;
typedef std::list<FBIStepElm> FBIStepsList;

class FBIProbe {
	public:
		uint64_t m_ProbeID;
		uint64_t m_ProbeOffset;
		uint8_t m_ProbeRegNum;
		uint32_t m_ProbeDataSize;

		FBIStepsList *m_Steps;

		void SetSteps(FBIStepsList *list)
		{
			TRACE("list = %p", list);
			m_Steps = list;
		}

		FBIProbe(uint64_t ProbeID, uint64_t ProbeOffset, uint8_t ProbeRegNum, uint32_t ProbeDataSize)
		{
			m_ProbeID = ProbeID;
			m_ProbeOffset = ProbeOffset;
			m_ProbeRegNum = ProbeRegNum;
			m_ProbeDataSize = ProbeDataSize;

			TRACE("->Create id=%lu; off=0x%lx reg=%d size=%d", ProbeID, ProbeOffset, ProbeRegNum, ProbeDataSize);
		}

		~FBIProbe()
		{
			TRACE("->Destroy id=%lu; off=0x%lx reg=%d size=%d", m_ProbeID, m_ProbeOffset, m_ProbeRegNum, m_ProbeDataSize);
			delete m_Steps;
		}
};

class CProbeDataFBIStep: public CProbeData
{
	public:
		uint8_t m_PointerOrder;
		uint64_t m_Offset;

		void accept(CVisitor& v) /* CNode */
		{
			if (v.access(this) == CVisitor::ACCESS_GRANTED) {
				v.entry(this);
				v.exit(this);
			}
		}

		virtual void printElm()
		{
			TRACE("po=%d off=0x%lx", m_PointerOrder, m_Offset);
		}

		virtual void printList()
		{
			printElm();
		}

		virtual int setData(CProbeData *data)
		{
			TRACE("set data");
			/* this element cannot contain data */
			return -EINVAL;
		}

		virtual int addData(CProbeData *data)
		{
			TRACE("add data");
			/* this element cannot contain data */
			return -EINVAL;
		}

		CProbeDataFBIStep(uint8_t PointerOrder, uint64_t Offset)
		:CProbeData(PROBE_DATA_FBI_STEP)
		{
			m_PointerOrder = PointerOrder;
			m_Offset = Offset;
			TRACE("->Create po=%d; off=0x%lx", PointerOrder, Offset);
		}

		virtual ~CProbeDataFBIStep()
		{
			TRACE("->Delete po=%d; off=0x%lx", m_PointerOrder, m_Offset);
		}

};
typedef std::shared_ptr<CProbeDataFBIStep> pCProbeDataFBIStep;

class CProbeDataFBIStepList : public CProbeDataList
{
	public:

		void accept(CVisitor& v) /* CNode */
		{
			if (v.access(this) == CVisitor::ACCESS_GRANTED) {
				v.entry(this);
				itList i;
				for (i = m_List->begin(); i != m_List->end(); i++)
					(*i)->accept(v);
				v.exit(this);
			}
		}

		virtual int addData(CProbeData *data)
		{
			TRACE("add data");
			if (data->m_DataType == PROBE_DATA_FBI_STEP) {
				TRACE("add data step");
				CProbeDataList::addData(data);
			} else {
				TRACE("add data ERR type = %d", data->m_DataType);
				return -EINVAL;
			}
		}

		CProbeDataFBIStepList()
		:CProbeDataList(PROBE_DATA_FBI_STEP_LIST)
		{
		}

		virtual ~CProbeDataFBIStepList()
		{
		}
};

class CProbeDataFBIVar: public CProbeData
{
	public:
		uint64_t m_ProbeID;
		uint64_t m_ProbeOffset;
		uint8_t m_ProbeRegNum;
		uint32_t m_ProbeDataSize;

		CProbeData *m_Steps;

		virtual void accept(CVisitor& v) /* CNode */
		{
			if (v.access(this) == CVisitor::ACCESS_GRANTED) {
				v.entry(this);
				m_Steps->accept(v);
				v.exit(this);
			}
		}

		virtual void printElm()
		{
			TRACE("id=%ld off=0x%lx reg=%d size=%d",
			      m_ProbeID,
			      m_ProbeOffset,
			      m_ProbeRegNum,
			      m_ProbeDataSize);
		}
		virtual void printList()
		{
			printElm();
			m_Steps->printList();
		}

		virtual int setData(CProbeData *data)
		{
			TRACE("set data");
			m_Steps = data;
		}

		virtual int addData(CProbeData *data)
		{
			TRACE("add data");
			if (data->m_DataType == PROBE_DATA_FBI_STEP) {
				m_Steps->addData(data);
			} else {
				TRACE("ERROR wrong data type %d", data->m_DataType);
				return -EINVAL;
			}
		}

		CProbeDataFBIVar(uint64_t ProbeID, uint64_t ProbeOffset,
				 uint8_t ProbeRegNum, uint32_t ProbeDataSize)
		:CProbeData(PROBE_DATA_FBI_VAR)
		{
			m_ProbeID = ProbeID;
			m_ProbeOffset = ProbeOffset;
			m_ProbeRegNum = ProbeRegNum;
			m_ProbeDataSize = ProbeDataSize;
			m_Steps = new CProbeDataFBIStepList();

			TRACE("->Create id=%ld; off=0x%lx reg=%d size=%d", ProbeID, ProbeOffset, ProbeRegNum, ProbeDataSize);
		}

		~CProbeDataFBIVar()
		{
			TRACE("->Delete id=%ld; off=0x%lx reg=%d size=%d", m_ProbeID, m_ProbeOffset, m_ProbeRegNum, m_ProbeDataSize);
			delete m_Steps;
		}

};

typedef std::shared_ptr<CProbeDataFBIVar> pCProbeDataFBIVar;

class CProbeDataFBIVarList : public CProbeDataList
{
	public:
		virtual int addData(CProbeData *data){
			CProbeData *var;
			TRACE("add data");
			switch(data->m_DataType) {
				case PROBE_DATA_FBI_VAR:
					CProbeDataList::addData(data);
					break;
				case PROBE_DATA_FBI_STEP:
					var = m_List->back();
					if (var != NULL) {
						var->addData(data);
					} else {
						TRACE("Cannot get last var");
						return -EINVAL;
					}
					break;
				default:
					return -EINVAL;
			}

			return 0;
		}

		virtual int setData(CProbeData *data)
		{
			CProbeDataList::setData(data);
		}

		void accept(CVisitor& v) /* CNode */
		{
			if (v.access(this) == CVisitor::ACCESS_GRANTED) {
				v.entry(this);
				itList i;
				for (i = m_List->begin(); i != m_List->end(); i++)
					(*i)->accept(v);
				v.exit(this);
			}
		}

		CProbeDataFBIVarList()
		:CProbeDataList(PROBE_DATA_FBI_VAR_LIST)
		{
		}

		virtual ~CProbeDataFBIVarList()
		{
		}
};

typedef CProbeDataFBIVarList * pCProbeDataFBIVarList;

#endif /* __CPROBE_DATA_FBI_VAR_H__ */
