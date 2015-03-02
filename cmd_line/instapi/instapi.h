#ifndef __ControlAPI_H__
#define __ControlAPI_H__

#include "CConfig.h"
#include "IInstrumentation.h"
#include <pthread.h>

namespace instapi
{

class CControlAPI
{
public:
	typedef shared_ptr<CConfig> pCConfig;

	CControlAPI ()
	{
		m_pConfig = pCConfig(new CConfig());
	}

	CControlAPI ( shared_ptr<CConfig>& pConfig ):
		m_pConfig ( pConfig )
	{
	}

	int startTrace();
	int stopTrace();
	int setConfig(const char *szConfig, std::shared_ptr<CConfig>& pConfig);
private:
	pCConfig m_pConfig;
	shared_ptr<IInstrumentation> m_pClient;

public:
       const char* GetLastError();
};

typedef std::shared_ptr<instapi::CControlAPI> pCControlAPI;

}
#endif /* __ControlAPI_H__ */
