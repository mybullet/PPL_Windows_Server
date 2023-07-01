#include "TcpServer.h"
#include <iostream>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <vector>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")

TcpServer::TcpServer(std::string ip, unsigned short tcpPort):m_ipAddress(ip), m_tcpPort(tcpPort)
{

}


TcpServer::~TcpServer()
{
	m_running = false;
	
	m_sockThreadGroup.join_all();
}



bool TcpServer::InitSock()
{
	//��ʼ��winsock2.DLL
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		//std::cout << "����winsock.dllʧ�ܣ�" << std::endl;
		LogError("fail to load winsock.dll��error code:{}", WSAGetLastError());
		return false;
	}
	//�����׽���
	if ((m_listenSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR)
	{
		//std::cout << "�����׽���ʧ�ܣ�������룺" << WSAGetLastError() << std::endl;
		LogError("fail to create listen socket��error code:{}", WSAGetLastError());
		WSACleanup();
		return false;
	}
	//�󶨶˿ں�Ip
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_tcpPort);
	inet_pton(AF_INET, m_ipAddress.c_str(), &addr.sin_addr);//�󶨱����Ļ��ص�ַ
	if (SOCKET_ERROR == bind(m_listenSocket, (SOCKADDR*)&addr, sizeof(sockaddr_in)))
	{
		//std::cout << "��ַ��ʧ�ܣ�������룺" << WSAGetLastError() << std::endl;
		LogError3("fail to bind address {}-{} to listen socket! error code:{}", m_ipAddress, m_tcpPort, WSAGetLastError());
		closesocket(m_listenSocket);
		WSACleanup();
		return false;
	}

	LogInfo2("work server success to bind {}:{}", m_ipAddress, m_tcpPort);
	return true;
}


void TcpServer::Stop()
{
	m_running = false;
	closesocket(m_listenSocket);
	m_sockThreadGroup.join_all();
}


void TcpServer::Start()
{

	m_initialized = InitSock();
	m_running = true;
	m_clientNumber = 2;
	//���׽�����Ϊ����״̬
	listen(m_listenSocket, m_clientNumber); //���m_clientNumber���ͻ����׽���

	if (m_initialized)
	{
		LogInfo1("work server sets MAX CLIENT NUMBER {}", m_clientNumber);
		// �����ź�
		//signal(42);
	}
	else
	{
		LogError("fail to init work server");
		closesocket(m_listenSocket);
		WSACleanup();
		return;
	}
	
	//���߳�ѭ�����տͻ��˵�����
	while (m_initialized && m_running)
	{
		sockaddr_in addrClient;
		int len = sizeof(sockaddr_in);
		//���ճɹ�������clientͨѶ��socket

		//LogInfo("server socket is waiting for the client to connect");

		SOCKET con = accept(m_listenSocket, (SOCKADDR*)&addrClient, &len);
		
		char ipBuf[100] = { 0 };
		inet_ntop(AF_INET, &addrClient.sin_addr, ipBuf,100);
		/*char portBuf[100] = { 0 };
		inet_ntop(AF_INET, &addrClient.sin_port, portBuf, 100);*/
		LogInfo2("accept a client {} from {}", con, ipBuf);
	   

		if (con != INVALID_SOCKET)
		{
			m_sockThreadGroup.create_thread(boost::bind(&TcpServer::ClientComm, this, con));
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	closesocket(m_listenSocket);
	WSACleanup(); 
	LogInfo("work server is closed successfully");
}

std::tuple<ServiceCommondEnum, std::string, std::string> TcpServer::GetService(std::string recvStr)
{
	std::tuple<ServiceCommondEnum, std::string, std::string> ret;
	if (recvStr.find("RequestRecog") == 0)
	{
		std::vector<std::string> vStr;
		boost::split(vStr, recvStr, boost::is_any_of("_"), boost::token_compress_on);
		vStr[0].erase(vStr[0].begin(), vStr[0].begin() + strlen("RequestRecog"));
		
		char tt[20] = { 0 };
		time_t timep;
		tm p;
		time(&timep); //��ȡ1900��1��1��0��0��0�뵽���ھ���������
		localtime_s(&p, &timep); //������ת��Ϊ����ʱ��,���1900����,��Ҫ+1900,��Ϊ0-11,����Ҫ+1
		memset(tt, 0, sizeof(tt));
		sprintf_s(tt, "%02d%02d%02d_", p.tm_hour, p.tm_min, p.tm_sec);
		std::string fileName = tt;

		for (std::vector<std::string>::iterator it = vStr.begin() + 1; it != vStr.end(); it++)
		{
			fileName += *it;
		}

		ret = { ServiceCommondEnum::RequestRecog, vStr[0], fileName };
	}
	else if (recvStr.find("GetRecog") == 0)
	{
		ret = { ServiceCommondEnum::GetRecog, "", "" };
	}
	else if (recvStr.find("RequestSelect") == 0)
	{
		std::vector<std::string> vStr;
		boost::split(vStr, recvStr, boost::is_any_of("_"), boost::token_compress_on);
		
		std::string selectStatement;
		for (std::vector<std::string>::iterator it = vStr.begin() + 1; it != vStr.end(); it++)
		{
			selectStatement += *it;
		}
		ret = { ServiceCommondEnum::RequestSelect, selectStatement, "" };
	}
	else if (recvStr.find("GetSelect") == 0)
	{
		ret = { ServiceCommondEnum::GetSelect, "", "" };
	}
	return ret;
}

//�߳�ͨѶ����
void TcpServer::ClientComm(SOCKET clientCon)
{
	bool bContinueAccept = false;
	long long fileSize;
	std::string fileName;
	std::string selectState;
	long long accpetedSize;
	FILE* fp = nullptr;
	
	//��ͻ���ͨѶ�����ղ��з�
	SOCKET sock = clientCon;
	//LogInfo1("sucess to build connection with {}", sock);
	char* recMsgBuffer = new char[64 * 1024];
	char* resultBin = new char[1 * 1024];
	while (m_running)
	{	
		//���տͻ�������
		memset(recMsgBuffer, 0, 64 * 1024);
		//LogInfo1("server is waiting for the new message from client {}", sock);
		int recvRet = recv(sock, recMsgBuffer, 64 * 1024, 0);
		if (recvRet == SOCKET_ERROR || recvRet == 0)
		{
			LogInfo1("client {} has closed the connection", sock);
			break;
		}
		else
		{
			if (bContinueAccept)
			{				
				if (fp != nullptr)
				{
					fwrite(recMsgBuffer, 1, recvRet, fp);//��buf��Ķ��������ļ���
					//LogInfo1("server is receiving the file resource from client {}", sock);
				}
							
				accpetedSize += recvRet;
				//LogInfo4("receiving resource from client {}, total:{}, receiving:{}, received:{}",sock, fileSize, recvRet, accpetedSize);
				if (accpetedSize == fileSize)
				{
					
					if (fp != nullptr)
					{
						fclose(fp);//��ɣ��ر��ļ�						
						fp = nullptr;
						
						//LogInfo1("finish to receive the file resource from client {}", sock);
					}
									
					int size = send(sock, "ConfirmRecog", sizeof("ConfirmRecog"), 0);//���ͻ��˷���һ����Ϣ
					if (size == SOCKET_ERROR || size == 0)
					{
						
						LogError3("fail to send {}, and stop connection with client {}, error code:{}", "ConfirmRecog", sock, WSAGetLastError());
						break;
					}
					else
					{
						g_scheduler.AddTask(fileName);
						//LogInfo4("add task:{} to g_scheduler:{}, and reply {} to client {}", fileName, g_scheduler.m_TaskAccpetMap.size(), "ConfirmRecog", sock);

					}
					bContinueAccept = false;
				}				
			}
			else
			{
				std::string splitPackage = recMsgBuffer;
				//LogInfo2("receive request from client {} : {}",sock, splitPackage);
				
				ServiceCommondEnum tempCommand;
				std::string tempPara1, tempPara2;
				std::tie(tempCommand, tempPara1, tempPara2) = GetService(splitPackage);

				if (tempCommand == ServiceCommondEnum::RequestRecog)
				{
					LogInfo2("handle request from client {}:{}", sock, "RequestRecog");
					fileSize = atoi(tempPara1.c_str());
					fileName = tempPara2;
					bContinueAccept = true;
					accpetedSize = 0;

					fopen_s(&fp, ("./Images/" + fileName).c_str(), "wb");//���ļ�������������ļ�

					//LogInfo3("prepare to receive file resource from client {}, file size:{}, file name:{}", sock, fileSize, fileName);
				}
				else if (tempCommand == ServiceCommondEnum::GetRecog)
				{
					LogInfo2("handle request from client {}:{}", sock, "GetRecog");
					std::string ocrResult = g_scheduler.GetResult(fileName);
					if (ocrResult != "")
					{
						
						std::string displayStr;
						std::ifstream inFile;
						memset(resultBin, 0, 1 * 1024);
						inFile.open("./Ocr_Results/" + ocrResult + ".txt", std::ios::in | std::ios::binary);
						if (!inFile) 
						{
							displayStr = "system error, fail to recog";
							sprintf_s(resultBin, 1 * 1024, "%s", displayStr.c_str());				
							LogError2("unable to open {}, client is {}", ocrResult, sock);
						}
						else
						{
							
							inFile.read(resultBin, 1 * 1024);
							inFile.close();
							displayStr = UTF8ToGB(resultBin);
						}
						
						if (displayStr.length() == 0)
						{
							displayStr = "Too vague to identify!";
							sprintf_s(resultBin, 1 * 1024, "%s", displayStr.c_str());
						}

						int size = send(sock, resultBin, 1 * 1024, 0);//���ͻ��˷���һ����Ϣ
						if (size == SOCKET_ERROR || size == 0)
						{
							LogError2("fail to send the result of recog, and stop connection with client {}, error code:{}", sock, WSAGetLastError());
							break;;
						}
						else
						{
							//LogInfo2("send the result of recog to client {},:{}", sock, displayStr);
							g_scheduler.CloseTask(fileName);
							//LogInfo2("close task:{}, and current g_scheduler is {}", fileName, g_scheduler.m_TaskAccpetMap.size());
						}

					}
					else
					{
						int size = send(sock, "WaitRecog", sizeof("WaitRecog"), 0);//���ͻ��˷���һ����Ϣ
						if (size == SOCKET_ERROR || size == 0)
						{
							LogError3("fail to send {}, and stop connection with client {}, error code:{}", "WaitRecog", sock, WSAGetLastError());
							break;
						}
						else
						{
							//LogInfo3("reply {} to client {}, and current g_scheduler:{}", "ConfirmRecog", sock, g_scheduler.m_TaskAccpetMap.size());
						}

					}
				}
				else if (tempCommand == ServiceCommondEnum::RequestSelect)
				{
					LogInfo2("handle request from client {}:{}", sock, "RequestSelect");
					selectState = tempPara1;
					int size = send(sock, "ConfirmSelect", sizeof("ConfirmSelect"), 0);//���ͻ��˷���һ����Ϣ
					if (size == SOCKET_ERROR || size == 0)
					{
						LogError3("fail to send {}, and stop connection with client {}, error code:{}", "ConfirmSelect", sock, WSAGetLastError());
						break;
					}
					else
					{
						//LogInfo2("server success to send {} to client {}", "ConfirmSelect", sock);
					}
				}
				else if (tempCommand == ServiceCommondEnum::GetSelect)
				{
					LogInfo2("handle request from client {}:{}", sock, "GetSelect");
					std::list<std::tuple<std::string, std::string, std::string> > resultList = g_dataSaver.SelectByDT(selectState);
					
					std::string displayStr;
					for (std::list<std::tuple<std::string, std::string, std::string> > ::iterator it = resultList.begin(); it != resultList.end(); it++)
					{					
						std::ifstream inFile;
						memset(resultBin, 0, 1 * 1024);
						inFile.open("./Ocr_Results/" + std::get <2>(*it) + ".txt", std::ios::in | std::ios::binary);
						if (!inFile)
						{
							displayStr = "system error, fail to recog";
							sprintf_s(resultBin, 1 * 1024, "%s", displayStr.c_str());
							LogError2("unable to open {}, client is {}", std::get <2>(*it), sock);						
						}
						else
						{

							inFile.read(resultBin, 1 * 1024);
							inFile.close();
							displayStr += UTF8ToGB(resultBin);
						}

					}

					int size;
					if (displayStr.empty())
					{
						size = send(sock, "there is no information at this moment!", sizeof("there is no information at this moment!"), 0);//���ͻ��˷���һ����Ϣ
						displayStr = "there is no information at this moment!";
					}
					else
					{
						size = send(sock, resultBin, 1 * 1024, 0);//���ͻ��˷���һ����Ϣ
					}
					
					if (size == SOCKET_ERROR || size == 0)
					{
						LogError2("fail to send the result of select, and stop connection with client {}, error code:{}", sock, WSAGetLastError());
						break;
					}
					else
					{
						//LogInfo2("server success to send select result to client {},:{}", sock, displayStr);
					}
						
				}
			}
			

		}
			
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	delete[] recMsgBuffer;
	delete[] resultBin;

	LogInfo1("success to close connection with client {}", sock);
}

std::string TcpServer::UTF8ToGB(const char* str)
{
	std::string result;
	WCHAR* strSrc;
	LPSTR szRes;

	//�����ʱ�����Ĵ�С
	int i = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	strSrc = new WCHAR[i + 1];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, strSrc, i);

	//�����ʱ�����Ĵ�С
	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
	szRes = new CHAR[i + 1];
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	result = szRes;
	delete[]strSrc;
	delete[]szRes;

	return result;
}
