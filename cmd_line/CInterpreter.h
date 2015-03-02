#ifndef __CINTERPRETER_H__
#define __CINTERPRETER_H__

#include "CConfig.h"
#include "instapi.h"

using namespace std;
using namespace instapi;

enum EMode
{
	ALL,
	CONF_ONLY
};

int YYmain(pCConfig &pConfig, pCControlAPI &pCControlAPI,
	   FILE *pFd_in, FILE *pFd_out, EMode nMode);

class CInterpreter
{
	public:
		CInterpreter(pCConfig &pConfig, EMode nMode,
			     FILE *pFD_in, FILE *pFD_out)
		: m_nMode(nMode), m_pFD_in(pFD_in), m_pFD_out(pFD_out)
		{
			if (pFD_out == NULL) {
				TRACE("set FD_out to stdout %p", stdout);
				m_pFD_out = stdout;
			}
			if (pFD_in == NULL) {
				TRACE("set FD_out to stdin %p", stdin);
				m_pFD_in = stdin;
			}


			m_pControl = pCControlAPI(new instapi::CControlAPI(pConfig));
			m_pConfig = pConfig;
		}

		void Interpret();

		virtual ~CInterpreter()
		{
		}

		int emergencyStopOfCollection()
		{
			return m_pControl->stopTrace();
		}

	protected:
		pCConfig m_pConfig;
		pCControlAPI m_pControl;
		EMode m_nMode;

		FILE * m_pFD_in;
		FILE * m_pFD_out;
};

int SaveConfig(pCConfig &pConfig, const char *filename);

#endif
