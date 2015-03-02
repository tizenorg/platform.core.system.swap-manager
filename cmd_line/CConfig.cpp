
#include <arpa/inet.h>

#include "CConfig.h"
#include "debug.h"
#include "defines.h"

/************** NEW ************/

CConfig::CConfig ()
{

/* new */
	m_AppList = new CAppListContainer();
}

CConfig::~CConfig()
{
	TRACE("Destroy");
	delete m_AppList;
	TRACE("end");
	clean();
}

void CConfig::clean()
{
}

CProbeListContainer *CConfig::getCurrentAppProbe()
{
	CAppListElm *app = NULL;
	if (m_AppList != NULL)
		app = m_AppList->getCurrent();
	if (app == NULL || app->m_Probes == NULL)
		return NULL;
	return app->m_Probes;
};

CProbeListContainer *CConfig::getCurrentLibProbe()
{
	CAppListElm *cur_app;
	CLibListElm *cur_lib;

	if (m_AppList == NULL)
		return NULL;

	cur_app = m_AppList->getCurrent();
	if (cur_app == NULL || cur_app->m_LibList == NULL)
		return NULL;

	cur_lib = cur_app->m_LibList->getCurrent();
	if (cur_lib == NULL)
		return NULL;
	return cur_lib->m_Probes;
};

int CConfig::trace (const char *message)
{
	TRACE("trace->%s", message);
}

int CConfig::setCurrentApplication (app_type_t app_type, const char *app_id, const char *app_path)
{
	int ret = 0;
	TRACE("Type %d, AppID '%s', AppPath '%s'", app_type, app_id, app_path);

	if (m_AppList == NULL) {
		/* TODO some error report */
		return -EINVAL;
	}

	m_AppList->setCurOrCreate(app_type, app_id, app_path);
	m_AppList->printList();

	return ret;
}

int CConfig::setCurrentLib (const char *lib_path)
{
	TRACE("LibPath '%s'", lib_path);

	CAppListElm *cur_app;

	cur_app = m_AppList->getCurrent();
	if (cur_app != NULL) {
		cur_app->m_LibList->setCurOrCreate(lib_path);
	} else {
		return -EINVAL;
	}

	m_AppList->printList();
	return 0;
}



int CConfig::setCurrentLibProbe (uint64_t addr, probe_t ptype, CProbeData *data)
{
	TRACE("Probe '0x%lx:%d'", addr, ptype);

	CProbeListContainer *probes = getCurrentLibProbe();
	CProbeListElm *probe = NULL;

	if (probes == NULL)
		return -EINVAL;

	probe = probes->setCurOrCreate(addr, ptype);
	if (probes == NULL)
		return -EINVAL;

	if (data != NULL)
		probe->setData(data);

	m_AppList->printList();
	return 0;
}

int CConfig::setCurrentLibProbe(uint64_t addr, probe_t ptype)
{
	return setCurrentLibProbe(addr, ptype, NULL);
}

int CConfig::setCurrentAppProbe(uint64_t addr, probe_t ptype, CProbeData *data)
{
	TRACE("Probe '0x%lx:%d'", addr, ptype);
	CProbeListContainer *probes = getCurrentAppProbe();
	CProbeListElm *probe = NULL;

	if (probes == NULL) {
		LOGE("Cannot get curr probe");
		return -EINVAL;
	}

	probe = probes->setCurOrCreate(addr, ptype);
	if (probe == NULL) {
		LOGE("Cannot set probe");
		return -EINVAL;
	}

	if (data != NULL)
		probe->setData(data);

	TRACE("list:");
	m_AppList->printList();
	return 0;
}

int CConfig::setCurrentAppProbe (uint64_t addr, probe_t ptype)
{
	return setCurrentAppProbe(addr, ptype, NULL);
}


int CConfig::setCurrentProbe(probe_level_type_t plevel, uint64_t addr, probe_t ptype, CProbeData *data)
{
	switch (plevel) {
		case PROBE_TYPE_LEVEL_LIB:
			return setCurrentLibProbe(addr, ptype, data);
		case PROBE_TYPE_LEVEL_APP:
			return setCurrentAppProbe(addr, ptype, data);
		default:
			LOGE("Wrong probe level type [" << plevel << "]");
			return -EINVAL;
	}
}

int CConfig::setCurrentProbe(probe_level_type_t plevel, uint64_t addr, probe_t ptype)
{
	return setCurrentProbe(plevel, addr, ptype, NULL);
}

int CConfig::addCurrentAppProbeData (CProbeData *data)
{
	TRACE("ProbeVar '%p'", data);

	CAppListElm *cur_app;

	cur_app = m_AppList->getCurrent();
	if (cur_app == NULL)
		return -EINVAL;

	cur_app->m_Probes->getCurrent()->addData(data);

	TRACE("list:");
	m_AppList->printList();
	return 0;
}

/**** GET *****/
int CConfig::getAll()
{
	TRACE("*****************************");
	m_AppList->printList();
}


