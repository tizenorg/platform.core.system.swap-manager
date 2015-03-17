#include <iostream>
#include <cerrno>
#include "CConfig.h"
#include "debug.h"

#ifndef __CCLIENT_H__
#define __CCLIENT_H__

using namespace std;

class SDB
{
	private:
		pCConfig m_Config;
		string m_SDBcmd;


	public:
		int cmd(string cmd_line)
		{
			int res;
			cmd_line = m_SDBcmd + cmd_line;
			LOGI("cmd: <"<< cmd_line << ">");
			res = system(cmd_line.c_str());
			return res;
		}

		int shell(string cmd_line)
		{
			return cmd("shell \"" + cmd_line + "\"");
		}

		int prepare()
		{
			int res = 0;
			m_SDBcmd = "sdb ";
			LOGI("sdb installed. host mode on. type = " << m_Config->getTargetType());
			switch (m_Config->getTargetType()) {
				case TT_AUTO:
					m_SDBcmd += "";
					break;
				case TT_DEVICE:
					m_SDBcmd += "-d ";
					break;
				case TT_VIRTUAL:
					m_SDBcmd += "-e ";
					break;
				default:
					LOGE("wrong target type" << m_Config->getTargetType());
					res = -EINVAL;
					break;
			}

			LOGI(m_SDBcmd);
			return res;
		}

		SDB(pCConfig conf)
		{
			m_Config = conf;
			prepare();
		}

		~SDB()
		{
		}
};

typedef std::shared_ptr<SDB> pSDB;

enum ClientStatus {
	CS_DISCONNECTED = 0,
	CS_CONNECTED,
	CS_PROFILING,
};
/*
class VirtualConnector
{
	protected:
		ClientStatus m_Status;
		pCConfig m_Config;
	public:
		virtual int send(const char *buf, size_t data_size)=0;
		virtual int recv(const char *buf, size_t data_size)=0;
		virtual int connect()=0;

		ClientStatus getStatus(){return m_Status;}

		VirtualConnector(pCConfig conf)
		{
			m_Status = CS_DISCONNECTED;
			m_Config = conf;
		}

		virtual ~VirtualConnector()
		{
		}
};*/

/* TODO UDS_NAME defined in da_manager use include from da_manager */
#define UDS_NAME				"/tmp/da.socket"
#include <sys/socket.h>		// for socket, connect
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>

typedef void (* changeStatusCb_t)(int);

class SocketClient
{
	protected:
		int m_Socket;
		ClientStatus m_Status;
		pCConfig m_Config;
		changeStatusCb_t m_StatusChangedCb;

		void setStatus(ClientStatus status)
		{
/*			if (m_StatusChangedCb) {
				m_StatusChangedCb(status);
			}*/
			LOGI("Set status " << status);
			m_Status = status;
		}
	public:
		virtual int send(const char *buf, size_t data_size)
		{
			LOGI(">");
			if (m_Status == CS_DISCONNECTED || m_Socket == -1) {
				LOGE("not connected");
				return -EINVAL;
			}

			::send(m_Socket, buf, data_size, MSG_NOSIGNAL);

			return 0;
		}

		virtual int recv(char *buf, size_t data_size)
		{
			if (m_Socket == CS_DISCONNECTED || m_Socket == -1) {
				LOGE("not connected");
				return -EINVAL;
			}
			::recv(m_Socket, buf, data_size, MSG_WAITALL);
			return 0;
		}

		virtual int connect()
		{
//			connect(in_addr *IP, int port)
			LOGE("not implemented");
			return -1;
		}

		virtual int connect(in_addr *IP, int port)
		{
			int sockfd;
			struct sockaddr_in serv_addr;

			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sockfd < 0)
				LOGE("ERROR opening socket");

			bzero((char *) &serv_addr, sizeof(serv_addr));
			serv_addr.sin_family = AF_INET;
			bcopy((char *)IP, (char *)&serv_addr.sin_addr,
			      sizeof(*IP));
			serv_addr.sin_port = htons(port);
			if (::connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
				LOGE("ERROR connecting");
				return -EINVAL;
			}

			m_Socket = sockfd;

			setStatus(CS_CONNECTED);
			return 0;
		}

		ClientStatus getStatus(){return m_Status;}
		//void setStatus(ClientStatus status){m_Status = status;}

/*		int setChangeStatusCb(changeStatusCb_t cb)
		{
			m_StatusChangedCb = cb;
		}
*/


		SocketClient(pCConfig conf)
		{
			m_Socket = -1;
			m_Status = CS_DISCONNECTED;
			m_Config = conf;
		}

		virtual ~SocketClient()
		{
		}
};

class SDBConnector: public SocketClient
{
	protected:
		pSDB m_SDB;
		int m_Socket;
	public:
		virtual int send(const char *buf, size_t data_size);
		virtual int recv(char *buf, size_t data_size);
		int connect();
		int connect(in_addr *IP, int port);

		SDBConnector(pCConfig conf, pSDB sdb)
		: SocketClient(conf), m_SDB(sdb)
		{
			m_Socket = -1;
		}

		~SDBConnector()
		{
		}
};

class VirtualLauncher
{
	protected:
		pCConfig m_Config;
	public:
		virtual int start() = 0;
		virtual int stop() = 0;

		VirtualLauncher(pCConfig conf)
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
		pSDB m_SDB;
	public:


		int start()
		{
			int res = 0;
			res += m_SDB->prepare();
			res += m_SDB->cmd("root on");
			res += m_SDB->shell("da_manager; sleep 3;");
			return res;
		}

		int stop()
		{
		}

		SDBLauncher(pCConfig conf, pSDB sdb)
		:VirtualLauncher(conf), m_SDB(sdb)
		{
		}

		virtual ~SDBLauncher()
		{
		}
};

#include "da_protocol.h"

class CClient
{
	protected:
		pCConfig m_Config;
		ClientStatus m_ClientStatus;
	public:
		SocketClient *m_Connection;
		VirtualLauncher *m_Launcher;

		void onConnectStatusChange(int i) {
			LOGI("OLOLO " << i);
		}

		int connect();

		ClientStatus getStatus(){return m_Connection->getStatus();}

		int applyConfig()
		{
			struct msg_t hdr;
			int res;
			res = connect();
			if (res != 0) {
				LOGE("Can not connect");
				goto exit_apply_conf;
			}

			char arr[1024];
			char *p;
			char *size;
			p  = (char *)arr;
			size = p;

			LOGI("p = " << (void *)p << "; size = " << (void *)size);

			*(uint32_t *)p = 0x0004; p += 4;
			LOGI("p = " << (void *)p << "; size = " << (void *)size);

			*(uint32_t *)p = 0x0000; 
			LOGI("p = " << (void *)p << "; size = " << (void *)size);
			size = p;
			LOGI("p = " << (void *)p << "; size = " << (void *)size);
			p += 4;
			LOGI("p = " << (void *)p << "; size = " << (void *)size);

			p += m_Config->m_Features->serialize(p);
			LOGI("p = " << (void *)p << "; size = " << (void *)size);
			*(uint32_t *)p = 100; p += 4;
			LOGI("p = " << (void *)p << "; size = " << (void *)size);
			*(uint32_t *)p = 50; p += 4;
			LOGI("p = " << (void *)p << "; size = " << (void *)size);

			*(uint32_t *)size = p - size - 4;
			LOGI("p = " << (void *)p << "; size = " << (void *)size);

			LOGI("size = " << *(uint32_t *)size);

			LOGI("call send");
			size_t len;
			len  = p - arr;
			m_Connection->send(arr, len);
			LOGI("call recv");
			m_Connection->recv((char *)&hdr, sizeof(hdr));

			exit_apply_conf:
			return res;
		}

		int startTrace()
		{
			int res;
			res = connect();
			if (res != 0) {
				LOGE("Can not connect");
				goto exit;
			}

			exit:
			return res;
		};



		CClient(pCConfig config);
		~CClient();
};

typedef std::shared_ptr<CClient> pCClient;

#endif /* __CCLIENT_H__ */
