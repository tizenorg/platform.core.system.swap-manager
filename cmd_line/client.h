#include <iostream>
#include <cerrno>
#include "debug.h"

#ifndef _CLIENT_H_
#define _CLIENT_H_

using namespace std;

enum m_TargetType_t {
	TT_AUTO,
	TT_DEVICE,
	TT_VIRTUAL
};

class Config
{
	private:
		int m_isConfiguredVal;
		m_TargetType_t m_TargetType;
	public:
		m_TargetType_t getTargetType() { return m_TargetType; };
		void setTargetType(m_TargetType_t type)
		{
			m_TargetType = type;
		};

		int loadFromFile(string file_name);
		Config();
		~Config();
};

class SDB
{
	private:
		m_TargetType_t m_TargetType;
		Config *m_Config;
	public:
		int cmd(string cmd_line)
		{
			return 0;
		}

		SDB(Config *conf)
		{
			m_Config = conf;
		}

		~SDB()
		{
		}
};

enum connection_status {
	CS_CONNECTED = 0,
	CS_DISCONNECTED = 1
};

class VirtualConnector
{
	protected:
		connection_status m_Status;
		Config *m_Config;
	public:
		virtual int send(const char *buf, size_t data_size)=0;
		virtual int recv(const char *buf, size_t data_size)=0;
		virtual int connect()=0;

		VirtualConnector(Config *conf)
		{
			m_Status = CS_DISCONNECTED;
			m_Config = conf;
		}

		virtual ~VirtualConnector()
		{
		}
};

class SDBConnector: public VirtualConnector
{
	protected:
	public:
		int send(const char *buf, size_t data_size);
		int recv(const char *buf, size_t data_size);
		int connect();

		SDBConnector(Config *conf)
		: VirtualConnector(conf)
		{
		}

		~SDBConnector()
		{
		}
};

class VirtualLauncher
{
	protected:
		Config *m_Config;
	public:
		virtual int prepare() = 0;
		virtual int start() = 0;
		virtual int stop() = 0;

		VirtualLauncher(Config *conf)
		{
			m_Config = conf;
		}

		~VirtualLauncher()
		{
		}
};

class SDBLauncher: public VirtualLauncher
{
	private:
		SDB *sdb;
	public:
		int prepare()
		{
			string sdb_cmd = "sdb ";
			LOGI("sdb installed. host mode on.");
			switch (m_Config->getTargetType()) {
				case TT_AUTO:
					sdb_cmd += "";
					break;
				case TT_DEVICE:
					sdb_cmd += "-d ";
					break;
				case TT_VIRTUAL:
					sdb_cmd += "-e ";
					break;
				default:
					LOGE("wrong target type" << m_Config->getTargetType());
					return -EINVAL;
					break;
			}

			LOGI(sdb_cmd);
		}

		int start()
		{
		}

		int stop()
		{
		}

		SDBLauncher(Config *conf)
		:VirtualLauncher(conf)
		{
			sdb = new SDB(conf);
		}

		~SDBLauncher()
		{
		}
};

class Client
{
	public:
		Config *m_Config;
		VirtualConnector *m_Connection;
		VirtualLauncher *m_Launcher;

		int connect();

		Client();
		~Client();
};

#endif /* _CLIENT_H_ */
