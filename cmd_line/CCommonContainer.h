
#include "CClient.h"
#include "CConfig.h"
#include "CInterpreter.h"
#include "instapi.h"


class CCommonContainer
{
	private:
		pCConfig m_Config;
		pCClient m_Client;
		CInterpreter *m_Inter;
		pCControlAPI m_ControlAPI;
	public:

		pCConfig getConfig()
		{
			return m_Config;
		}

		pCControlAPI getControlAPI()
		{
			return m_ControlAPI;
		}


		CInterpreter *getInter()
		{
			return m_Inter;
		}

		CCommonContainer(FILE *fin, FILE *fout)
		{
			m_Config = pCConfig(new CConfig());
			m_Client = pCClient(new CClient(m_Config));

			m_ControlAPI = pCControlAPI(new CControlAPI(m_Client));
			//m_ControlAPI->set
		
			//config->m_bPrompt = true;
			m_Inter = new CInterpreter(m_Config, ALL, fin, fout);
		
		/*
			//cl->config->load_from_file("some_file_name");
			cl->m_Launcher->prepare();
			if (cl->connect() != 0) {
				LOGE("can not connect");
			} else {
				LOGI("connected");
			}
			delete cl;
			delete _inter;
			*/
		}

		~CCommonContainer()
		{
			delete m_Inter;
		}
};

extern CCommonContainer *Global;
