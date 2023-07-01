#pragma once
//#include "TaskBase.h"
#include <string>
//#include <map>
#include <boost/thread/thread.hpp>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <functional>
#include "TaskSchedule.h"
#include "DataStorage.h"
#include "Logger.h"

extern TaskSchedule g_scheduler;
extern DataStorage g_dataSaver;


enum ServiceCommondEnum
{
	NoCommond = -1,
	RequestRecog = 0,
	GetRecog = 1,
	RequestSelect = 2,
	GetSelect = 3
};

class TcpServer
{
public:
	TcpServer(std::string ip = "10.0.3.224", unsigned short tcpPort = 65432);
	virtual ~TcpServer();
	void Start();
	void Stop();
	

	
protected:
	void ClientComm(SOCKET clientCon);

private:

	bool InitSock();
	std::tuple<ServiceCommondEnum, std::string, std::string> GetService(std::string recvStr);
	std::string UTF8ToGB(const char* str);

private:
	std::string m_ipAddress;
	unsigned short m_tcpPort;
	SOCKET m_listenSocket;
	bool m_initialized;
	bool m_running;
	
	boost::thread_group m_sockThreadGroup;
	
	int m_clientNumber;
};

