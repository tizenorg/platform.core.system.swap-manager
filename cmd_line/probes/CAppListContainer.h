#ifndef __CAPP_LIST_CONTAINER_H__
#define __CAPP_LIST_CONTAINER_H__

#ifdef __cplusplus

#include "CConfig.h"

/** Appication list classes **/
class CAppListElm : public CPrintable {
	public:
		app_type_t m_AppType;
		std::string m_AppID;
		std::string m_FilePath;
		CLibListContainer  *m_LibList;
		CProbeListContainer *m_Probes;

		virtual void printElm() /* CPrintable */
		{
			TRACE("Type %d, AppID '%s', AppPath '%s'", m_AppType, m_AppID.c_str(), m_FilePath.c_str());
		}

		virtual void printList() /* CPrintable */
		{
			printElm();
			m_Probes->printList();
			m_LibList->printList();
		}

		CAppListElm(app_type_t app_type, std::string app_id, std::string file_path)
		{
			m_AppType = app_type;
			m_FilePath = file_path;
			m_AppID = app_id;

			m_LibList = new CLibListContainer();
			m_Probes = new CProbeListContainer();
		}

		~CAppListElm() {
			TRACE("Destroy Type %d, AppID '%s', AppPath '%s'", m_AppType, m_AppID.c_str(), m_FilePath.c_str());
			delete m_LibList;
			delete m_Probes;
		}
};

typedef std::shared_ptr<CAppListElm> pAppListElm;
typedef std::list<pAppListElm> TAppList;
typedef TAppList::iterator itAppListElm;

class CAppListContainer: public CListContainer<CAppListElm>{
	public:

		virtual int elmCmp(pAppListElm a, pAppListElm b) {

			TRACE("%s", "Cmp>");
			a->printElm();
			b->printElm();
			int res = !
			   ((*a).m_AppType == (*b).m_AppType &&
			    (*a).m_AppID == (*b).m_AppID &&
			    (*a).m_FilePath == (*b).m_FilePath);

			TRACE("Cmp< %d", res);
			return res;
		}

		void setCurOrCreate(app_type_t app_type, const char *app_id, const char *app_path)
		{
			itAppListElm app;
			TRACE("Type %d, AppID '%s', AppPath '%s'", app_type, app_id, app_path);

			/* Search in application list */

			pAppListElm new_app(new CAppListElm(app_type, std::string(app_id), std::string(app_path)));
			_setCurOrCreate(new_app);

			for (app = m_List->begin(); app != m_List->end(); app++) {
				app->get()->printElm();
			}
		}
};


#endif /* __cplusplus */
#endif /* __CAPP_LIST_CONTAINER_H__ */
