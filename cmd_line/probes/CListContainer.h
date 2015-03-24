#ifndef _CLISTCONTAINER_H__
#define _CLISTCONTAINER_H__

#include "CVisitor.h"
#include "debug.h"

template <class TElm> class CListContainer: public CNode {
	public:
	typedef std::shared_ptr<TElm> pElm;
	typedef std::list<pElm> TList;
	typedef typename TList::iterator itList;

	protected:
		TList *m_List;
		pElm m_Current;

	public:
		virtual int elmCmp(pElm a, pElm b) = 0;

		itList _find(pElm app_elm)
		{
			itList app;
			for (app = m_List->begin(); app != m_List->end(); app++) {
				if (elmCmp(app_elm, *app) == 0)
					break;
			}
			return app;
		}

		TElm *setCurrent(pElm app)
		{
			m_Current = app;
			return &(*m_Current);
		}

		TElm *getCurrent()
		{
			if (m_Current != NULL)
				return &(*m_Current);
			return NULL;
		}

		TElm *_setCurOrCreate(pElm elm)
		{
			itList i;

			/* Search in application list */
			i = _find(elm);

			if (i == m_List->end()) {
				TRACE("%s", "Not found:");
				elm->printElm();
				m_List->push_back(elm);
				return setCurrent(elm);
			} else {
				return setCurrent(*i);
			}

		}

		virtual void printElm() /* CNode */
		{
		}
		virtual void printList() /* CNode */
		{
			itList i;
			for (i = m_List->begin(); i != m_List->end(); i++)
				i->get()->printList();
		}

		CListContainer()
		{
			m_List = new TList();
			m_Current = NULL;
		}

		~CListContainer()
		{
			m_Current = NULL;
			delete m_List;
		}

};


#endif /* _CLISTCONTAINER_H__ */
