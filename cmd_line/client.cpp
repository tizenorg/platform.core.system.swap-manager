#include <iostream>
#include <cstdlib>
#include <cerrno>

#include "client.h"
#include "debug.h"


using namespace std;


/* Client_config */
int Config::loadFromFile(string file_name)
{
	LOGI(file_name);
	return 0;
}

Config::Config()
{
	setTargetType(TT_AUTO);
	LOGI("bla");
}

Config::~Config()
{
	LOGI("bla");
}

/* SDBConnector */
int SDBConnector::connect()
{
	int res = 0;
	string sdb_dev;

	/* check location by sdb cmd FIXME */
	res = system("sdb devices");
	if (res == 0) {


	} else {
		LOGI("sdb not installed[%d]. standalone mode on");
	}

	return res;
}

int SDBConnector::send(const char *buf, size_t data_size)
{
	if (m_Status != CS_CONNECTED) {
		LOGE("cannot send data, no connection");
	}
	return 0;
}

int SDBConnector::recv(const char *buf, size_t data_size)
{
	if (m_Status != CS_CONNECTED) {
		LOGE("cannot recv data, no connection");
	}
	return 0;
}
/***************************************************************/

Client::Client()
{
	LOGI("bla");
	m_Config = new Config();
	m_Launcher = new SDBLauncher(m_Config);
	m_Connection = new SDBConnector(m_Config);
}

Client::~Client()
{
	LOGI("bla");
	delete m_Config;
}

int Client::connect()
{
	int res;
	m_Connection->connect();

	LOGI("bla");
	return 0;
}
