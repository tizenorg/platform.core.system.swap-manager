#ifndef __CPROBEDATALIST_H__
#define __CPROBEDATALIST_H__

#include "CProbeData.h"

class CProbeDataList  : public CProbeData
{
	public:
		typedef std::list<CProbeData *> pCProbeDataList;
		typedef pCProbeDataList::iterator itList;

		pCProbeDataList *m_List;

		virtual void printElm()
		{
			TRACE("------");
		}

		virtual void printList()
		{
			itList i;
			printElm();
			for (i = m_List->begin(); i != m_List->end(); i++) {
				(*i)->printList();
			}
		}

		virtual int setData(CProbeData *data)
		{
			TRACE("set data");
		}

		virtual int addData(CProbeData *data)
		{
			TRACE("add data [%d]", data->m_DataType);
			m_List->push_back(data);
		}

		CProbeDataList(datatype_t data_type)
		:CProbeData(data_type)
		{
			TRACE("create");
			m_List = new pCProbeDataList();
		}

		virtual ~CProbeDataList()
		{
			itList i;

			for (i = m_List->begin(); i != m_List->end(); i++) {
				delete (*i);
			}
			delete m_List;
		}


};


#endif /* __CPROBEDATALIST_H__ */
