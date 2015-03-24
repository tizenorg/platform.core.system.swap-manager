#ifndef __CLIB_LIST_CONTAINER_H__
#define __CLIB_LIST_CONTAINER_H__

#include "CVisitor.h"

/** Library list classes **/
class CLibListElm : public CNode {
	public:
		std::string m_LibPath;
		CProbeListContainer *m_Probes;

		virtual void printElm() /* CNode */
		{
			TRACE("LibPath '%s'", m_LibPath.c_str());
		}

		virtual void printList() /* CNode */
		{
			printElm();
			m_Probes->printList();
		}

		virtual void accept(CVisitor& v) /* CNode */
		{
			if (v.access(this) == CVisitor::ACCESS_GRANTED) {
				v.entry(this);
				m_Probes->accept(v);
				v.exit(this);
			}
		}

		CLibListElm(std::string lib_path)
		{
			m_LibPath = lib_path;
			m_Probes = new CProbeListContainer();
		}

		~CLibListElm()
		{
			delete m_Probes;
		}
};

typedef std::shared_ptr<CLibListElm> pLibListElm;
typedef std::list<pLibListElm> TLibList;
typedef TLibList::iterator itLibList;

class CLibListContainer: public CListContainer<CLibListElm>{
	public:
		virtual void accept(CVisitor& v) /* CNode */
		{
			itList i;
			v.entry(this);
			for (i = m_List->begin(); i != m_List->end(); i++)
				i->get()->accept(v);
			v.exit(this);
		}

		virtual int elmCmp(pLibListElm a, pLibListElm b)
		{
			return a.get()->m_LibPath != b.get()->m_LibPath;
		}

		CLibListElm *setCurOrCreate(const char *lib_path)
		{
			TRACE("LibPath '%s'", lib_path);

			pLibListElm new_lib(new CLibListElm(std::string(lib_path)));
			return _setCurOrCreate(new_lib);
		}
};


#endif /* __CLIB_LIST_CONTAINER_H__ */
