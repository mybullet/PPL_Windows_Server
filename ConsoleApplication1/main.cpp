#include <python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <boost/python.hpp>
#include <boost/thread/thread.hpp>
#include "Logger.h"
#include <iostream>
#include <thread>
#include <functional>
#include "TcpServer.h"
#include "TaskSchedule.h"
#include "OcrProcess.h"
#include "DataStorage.h"
#include <iomanip>
#include "Tools.h"
#include <regex>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <Windows.h>
#include <boost/signals2.hpp>

// 定义信号类型, 创建信号对象
boost::signals2::signal<void(std::string)> ZmqSignal;

#define TEST 0

using namespace std;



std::string ZmqMsgBuffer;
boost::shared_mutex ZmqMsgMutex;

TaskSchedule g_scheduler;
DataStorage g_dataSaver(true);

std::string SYSTEM_IS_RUNNING = "system is running!";


int main() 
{

    if (!InitialGlobalLogger(TEST))
    {
        std::cout << "Fail to start logger, exit!" << std::endl;
        return -1;
    }


    std::string localServerIP = GetLocalIpAddressFromIniFile();
    if (localServerIP.empty())
    {
        LogError("Failed to read local server address from CommConfig.ini file.");
        return -1;
    }

    //添加控制台交互逻辑
    if(0 != PrintAllNetAddrInfo())
    {
        LogError("Failed to print all net address info, exit!");
        return -1;
    }

    if (0 != SetTcpServerIpAddress(localServerIP))
    {
        LogError("Failed to set tcp server ip address, exit!");
        return -1;
    }

    std::string remoteServerIP = GetRemoteIpAddressFromIniFile();
    if (remoteServerIP.empty())
    {
        LogError("Failed to read remote server address from CommConfig.ini file.");
        return -1;
    }

    if (0 != SetZmqServerIpAddress(remoteServerIP))
    {
        LogError("Failed to set zmq server ip address, exit!");
        return -1;
    }

    
    TcpServer server(localServerIP);
    std::thread tcpListenThread(&TcpServer::Start, &server);
	
    StartOcrProcess();

    std::thread dataStorageThread(&DataStorage::Start, &g_dataSaver);

    const char* processName = "UdpClient.exe";

    if (KillProcessByName(processName)) 
    {
        LogInfo("Success to clear UdpClient process.");
    }
    else 
    {
        LogError("Failed to kill UdpClient process.");
        return -1;
    }

    std::string processPath = ".\\UdpClient\\UdpClient.exe";

    if (StartProcessByPath(processPath))
    {
        LogInfo("Successfully started UdpClient.exe");
    }
    else
    {
        LogError("Failed to start UdpClient.exe");
        return -1;
    }

    LogInfo(SYSTEM_IS_RUNNING);


    MainThread(remoteServerIP);


    StopOcrProcess();
    server.Stop();
    g_dataSaver.Stop();
	tcpListenThread.join();
    dataStorageThread.join();
    DestoryGlobalLogger();
    exit(120);
    return 0;
}
