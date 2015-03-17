#ifndef __ControlAPI_H__
#define __ControlAPI_H__

#include "CConfig.h"
#include "CClient.h"
#include "IInstrumentation.h"
#include <pthread.h>


class CControlAPI
{

private:
	pCConfig m_pConfig;
	pCClient m_pClient;

public:
//	typedef shared_ptr<CConfig> pCConfig;

//	CControlAPI (pCConfig& pConfig, pCClient& pClient)
	CControlAPI (pCClient& pClient)
	:m_pClient(pClient)
	{
	}

	int startTrace()
	{
		return m_pClient->startTrace();
	};

	int applyConfig()
	{
		return m_pClient->applyConfig();
	};


	int stopTrace();
	int setConfig(const char *szConfig, pCConfig& pConfig);

	/* TODO ??? */
	const char* GetLastError();
};

typedef std::shared_ptr<CControlAPI> pCControlAPI;

#endif /* __ControlAPI_H__ */
