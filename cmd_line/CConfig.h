#ifndef __CPROFILE_H__
#define __CPROFILE_H__

#ifdef __cplusplus

#include <netinet/in.h>
#include <list>
#include <memory>

#include "da_protocol.h"
#include "consts.h"
#include "CListContainer.h"
#include "CProbe.h"
#include "debug.h"
#include "defines.h"


#endif

#define ANALYTICS_LIST_FILE "analytics_names.txt"

#define COMMON_TRACE 1
#define ONLY_ENTRY_TRACE 2

#ifdef __cplusplus

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

#include "CAppListContainer.h"
class CConfig
{
public:
	CAppListContainer *m_AppList;
    /* methods */
	CConfig();
	virtual ~CConfig();
	int trace (const char* message);
	void clean();

	CProbeListContainer *getCurrentAppProbe();
	CProbeListContainer *getCurrentLibProbe();

	/* Probe methods */
	int setCurrentApplication(app_type_t app_type, const char *app_id, const char *app_path);
	int setCurrentLib (const char *lib_path);

	int setCurrentAppProbe (uint64_t addr, probe_t ptype);
	int setCurrentAppProbe (uint64_t addr, probe_t ptype, CProbeData *data);
	int setCurrentLibProbe (uint64_t addr, probe_t ptype);
	int setCurrentLibProbe (uint64_t addr, probe_t ptype, CProbeData *data);
	int setCurrentProbe(probe_level_type_t plevel, uint64_t addr, probe_t ptype, CProbeData *data);
	int setCurrentProbe(probe_level_type_t plevel, uint64_t addr, probe_t ptype);

	int addCurrentAppProbeData (CProbeData *data);
	void deleteLibProbes();
		/* get */
	int getAll();

private:
    static int MakeAbsolutePath(const char *path, std::string &out)
    {
        char tmp[PATH_MAX + 1];

        if (realpath(path, tmp) == NULL) {
        return -1;
        }
        out = tmp;

        return 0;
    }
};

typedef std::shared_ptr<CConfig> pCConfig;

#endif

#endif
