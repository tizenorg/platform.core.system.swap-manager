#ifndef __CLIB_LIST_CONTAINER_H__
#define __CLIB_LIST_CONTAINER_H__

#include "CListContainer.h"

/** Library list classes **/
class CLibListElm : public CPrintable {
	public:
		std::string m_LibPath;
		CProbeListContainer *m_Probes;

		virtual void printElm() /* CPrintable */
		{
			TRACE("LibPath '%s'", m_LibPath.c_str());
		}

		virtual void printList() /* CPrintable */
		{
			printElm();
			m_Probes->printList();
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

//typedef CListContainer<CAppListElm> CTest;

class CLibListContainer: public CListContainer<CLibListElm>{
	public:

		virtual int elmCmp(pLibListElm a, pLibListElm b) {
			return a.get()->m_LibPath != b.get()->m_LibPath;
		}

		CLibListElm *setCurOrCreate(const char *lib_path)
		{
			itLibList lib;
			TRACE("LibPath '%s'", lib_path);

			pLibListElm new_lib(new CLibListElm(std::string(lib_path)));
			return _setCurOrCreate(new_lib);
		}
};


#endif /* __CLIB_LIST_CONTAINER_H__ */
