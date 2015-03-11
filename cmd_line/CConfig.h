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

#include "CFeatures.h"
#include "CAppListContainer.h"

class CConfig
{
public:
	CFeatures *m_Features;
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

	int setFeature(feature_code f, bool enable = true);
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
