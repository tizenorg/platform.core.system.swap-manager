#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(STANDALONE)
#include "CAgent.h"
#else // defined(STANDALONE)
#include "CSmartClient.h"
#endif // defined(STANDALONE)
#include "IInstrumentation.h"
#include "CFile.h"
#include "instapi.h"
#include "symbolapi.h"
#include "event_tmpl.h"
#include "stdswap.h"

extern CTraceSaver g_traceSaver;

namespace instapi
{

const char* CControlAPI::GetLastError()
{
	return NULL;
}

int CControlAPI::StartTrace()
{
	return 0;
}

int CControlAPI::StopTrace()
{
	return 0;
}

int CControlAPI::SetProfile(const char *szProfile, smart_ptr<CProfile>& pProfile)
{
	int retval = 0;

	if (m_pClient != NULL) {
		return -1;
	}
	smart_ptr<IInstrumentation> _pClient;

#if defined(STANDALONE)
	_pClient = new CAgent();
#else	// defined(STANDALONE)
	_pClient = new CSmartClient();
#endif	// defined(STANDALONE)

	// TODO: What should we initialize? Is it necessary?
	// if (g_pClient->Initialize() != 0) {
	// 	TRACE("failed to initialize client!");
	// 	return -1;
	// }
	ASSERT(_pClient!=NULL);
	retval = _pClient->SetProfile(szProfile, *pProfile);
	if ( retval != 0) {
		TRACE("failed to set profile!");
		m_pClient = _pClient;
		return retval;
	}
	g_traceSaver.setEventMask(pProfile->m_nEventMask);
	m_pProfile = pProfile;
	m_pClient = _pClient;
	return 0;
}

}
