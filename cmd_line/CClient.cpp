#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <fstream>

#include "CClient.h"
#include "debug.h"


using namespace std;

#define PORT_FILE_NAME "port.da"
#define PORT_FILE "/tmp/" PORT_FILE_NAME

/* SDBConnector */

int SDBConnector::connect(in_addr *IP, int port)
{
	string port_str;
	port_str = to_string(port);

	LOGI("ip = " << to_string(IP->s_addr) << "; port = " << port_str);
	if (m_SDB->cmd("forward tcp:" + port_str + " tcp:" + port_str) != 0) {
		LOGE("Cannot forward ports");
		return -EINVAL;
	};

	return SocketClient::connect(IP, port);
}

int SDBConnector::connect()
{
	int res = 0;
	int port = -1;
	string port_str;
	struct in_addr IP;

	LOGI("connect");
	/* check config IP*/
	m_Config->getTargetIP(&IP);
	/* check config port*/
	port = m_Config->getTargetPort();

	if (port == 0) {
		/* get remote port */
		if (m_SDB->cmd("pull " PORT_FILE) != 0) {
			LOGE("cannot get port file");
			return -EINVAL;
		}

		ifstream f(PORT_FILE_NAME);

		if (f.is_open()) {
			if (getline(f, port_str)) {
				LOGI(port_str);
				port = atoi(port_str.c_str());
			} else {
				return -EINVAL;
			}
			f.close();
		} else {
			LOGE("cannot open port file");
			return -EINVAL;
		}
	}


	SDBConnector::connect(&IP, port);
	return 0;
}

int SDBConnector::send(const char *buf, size_t data_size)
{
	return SocketClient::send(buf, data_size);
	if (m_Status != CS_CONNECTED) {
		LOGE("cannot send data, no connection");
	}
	return 0;
}

int SDBConnector::recv(char *buf, size_t data_size)
{
	return SocketClient::recv(buf, data_size);
	if (m_Status != CS_CONNECTED) {
		LOGE("cannot recv data, no connection");
	}
	return 0;
}

/***************************************************************/

CClient::CClient(pCConfig config)
{
	int res;
	LOGI("bla");

	m_ClientStatus = CS_DISCONNECTED;
	m_Config = config;

	res = system("sdb devices");
	if (res == 0) {
		LOGI("sdb installed[%d]. host mode on");
		pSDB sdb = pSDB(new SDB(m_Config));
		m_Launcher = new SDBLauncher(m_Config, sdb);
		m_Connection = new SDBConnector(m_Config, sdb);
		//m_Connection->setChangeStatusCb(&CClient::onConnectStatusChange);
	} else {
		LOGI("sdb not installed[%d]. standalone mode on");
	}

}

CClient::~CClient()
{
	LOGI("bla");
	delete m_Launcher;
	delete m_Connection;
}

int CClient::connect()
{
	int res;

	if (m_Connection->getStatus() == CS_CONNECTED) {
		LOGI("already connected");
		goto exit;
	}

	LOGI("bla");
	res = m_Launcher->start();
	if (res != 0) {
		LOGE("Can not launch da_manager");
		goto exit;
	}

	res = m_Connection->connect();
	if (res != 0) {
		LOGE("Can not connect to da_manager");
		goto exit;
	}

exit:
	return res;
}
