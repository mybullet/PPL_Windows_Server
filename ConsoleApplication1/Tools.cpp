#include "Tools.h"
#include <io.h>
#include <direct.h>
#include <ctime>
#include "Logger.h"
#include <algorithm>
#include <sstream>
#include "zmq.hpp"

std::string encrypt(const std::string& message, const std::string& key) 
{
    // ��������������
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, reinterpret_cast<const unsigned char*>(key.c_str()), NULL);

    // ��������
    int cipherLen = message.length() + EVP_MAX_BLOCK_LENGTH;
    unsigned char* cipherText = new unsigned char[cipherLen];
    int len;
    EVP_EncryptUpdate(ctx, cipherText, &len, reinterpret_cast<const unsigned char*>(message.c_str()), message.length());
    cipherLen = len;
    EVP_EncryptFinal_ex(ctx, cipherText + len, &len);
    cipherLen += len;

    // �����ͷ���Դ
    EVP_CIPHER_CTX_free(ctx);

    // ���ؼ��ܺ������
    std::string encryptedText(reinterpret_cast<char*>(cipherText), cipherLen);
    delete[] cipherText;
    return encryptedText;
}

std::string decrypt(const std::string& encryptedMessage, const std::string& key) 
{
    // ��������������
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, reinterpret_cast<const unsigned char*>(key.c_str()), NULL);

    // ��������
    int plainLen = encryptedMessage.length();
    unsigned char* plainText = new unsigned char[plainLen];
    int len;
    EVP_DecryptUpdate(ctx, plainText, &len, reinterpret_cast<const unsigned char*>(encryptedMessage.c_str()), encryptedMessage.length());
    plainLen = len;
    EVP_DecryptFinal_ex(ctx, plainText + len, &len);
    plainLen += len;

    // �����ͷ���Դ
    EVP_CIPHER_CTX_free(ctx);

    // ���ؽ��ܺ������
    std::string decryptedText(reinterpret_cast<char*>(plainText), plainLen);
    delete[] plainText;
    return decryptedText;
}

void MainThread(std::string zmqAddress)
{
    //start zmq client to connect remote server for keeping alive
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::req);
    std::string zmqCommStr = "tcp://" + zmqAddress + ":7777";
    removeSpaces(zmqCommStr);
    //std::cout << "before connect : " << zmqCommStr << std::endl;
    socket.connect(zmqCommStr.c_str());
    //std::cout << "after connect" << std::endl;

    int timeout = 30;  // ��������Ӧ�ĳ�ʱʱ�䣨�룩
    int counter = 0;   // ����������¼��������Ӧ��ʱ��
    bool zmqRunning = true;
    bool needGetSecretKey = true;
    std::string secretKey = "";
    int wait_time = 500;

    while (zmqRunning)
    {
        try
        {
            // �ж��Ƿ����ӵ������
            if (socket.handle() == nullptr)
            {
                //std::cout << "reconnecting......." << std::endl;
                socket = zmq::socket_t(context, zmq::socket_type::req);  // �����µ� socket ����
                socket.connect(zmqCommStr.c_str());  // �����µ�����
                //std::cout << "Reconnected to the server" << std::endl;
            }

            std::string message;
            if (needGetSecretKey)
            {
                message = "GET@";

                zmq::message_t request(message.size());
                memcpy(request.data(), message.data(), message.size());

                // ��������������
                socket.send(request, zmq::send_flags::none);
            }
            else
            {
                std::string new_msg = GetZmqMessage();
                if(new_msg.empty())
                {
                    message = "LOCAL@" + SYSTEM_IS_RUNNING;
                    wait_time = 1000;
                }                    
                else
                {
                    message = "LOCAL@" + new_msg;
                }                   

                message = encrypt(message, secretKey);
                zmq::message_t request(message.size());
                memcpy(request.data(), message.data(), message.size());

                // ��������������
                socket.send(request, zmq::send_flags::none);
            }
            
            
            //std::cout << "Success to send hello server!" << std::endl;

            // �ȴ������շ���˵Ļظ������ȴ�500����
            zmq::pollitem_t items[] = { { socket, 0, ZMQ_POLLIN, 0 } };
            zmq::poll(items, 1, std::chrono::milliseconds(500));

            if (items[0].revents & ZMQ_POLLIN)
            {
                zmq::message_t reply;
                socket.recv(reply, zmq::recv_flags::none);
                std::string response(static_cast<char*>(reply.data()), reply.size());

                if (needGetSecretKey)
                {
                    // ���ظ���Ϣ�Ƿ���� "SECRET" ǰ׺
                    if (response.find("KEY@") == 0)
                    {
                        secretKey.clear();
                        secretKey = response.substr(std::string("KEY@").length());
                        if (LENGTH_SECRET_KEY == secretKey.length())
                        {
                            std::cout << "received secret key from server: " << secretKey << std::endl;
                            needGetSecretKey = false;
                        }
                                               
                    }

                    counter = 0;  // ���ü�����
                }
                else
                {
                    std::cout << "Received origin reply from server: " << response << std::endl;
                    response = decrypt(response, secretKey);

                    // ���ظ���Ϣ�Ƿ���� "REMOTE" ǰ׺
                    if (response.find("REMOTE@") != 0)
                    {
                        std::cout << "received invalid reply from server: " << response << std::endl;
                        counter++;  // �������ۼ�
                    }
                    else
                    {
                        std::cout << "Received reply from server: " << response << std::endl;

                        counter = 0;  // ���ü�����
                    }
                }

                
            }
            else
            {
                std::cout << "receiving is timeout!" << std::endl;
                counter++;  // �������ۼ�
            }

            if (0 != counter)
            {
                std::cout << "Counter is: " << counter << std::endl;
            }

            // �ж��Ƿ�ﵽ��ʱʱ��
            if (counter >= timeout)
            {
                std::cout << "crash!" << std::endl;
                zmqRunning = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
            wait_time = 500;
        }
        catch (zmq::error_t ex)
        {
            //std::cout << "throw an exception :" << ex.what() << std::endl;
            socket.close();  // �ر�֮ǰ������
            secretKey.clear();
            needGetSecretKey = true;
            counter++;  // �������ۼ�

            if (0 != counter)
            {
                std::cout << "Counter is: " << counter << std::endl;
            }

            // �ж��Ƿ�ﵽ��ʱʱ��
            if (counter >= timeout)
            {
                std::cout << "crash!" << std::endl;
                zmqRunning = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
            wait_time = 500;
        }

    }
    context.shutdown();
}

std::string ConvertSpdStr2Boost(const std::string& ss) 
{
    std::string result = ss;
    size_t pos = result.find("{}");
    int count = 1;

    while (pos != std::string::npos) {
        result.replace(pos, 2, "%" + std::to_string(count) + "%");
        pos = result.find("{}", pos + 2);
        count++;
    }

    return result;
}

// ����ۺ���
void UpdateZmqMessage(std::string msg)
{
    boost::unique_lock<boost::shared_mutex> lock(ZmqMsgMutex);
    ZmqMsgBuffer.clear();
    ZmqMsgBuffer = msg;
}

std::string GetZmqMessage()
{
    //boost::shared_lock<boost::shared_mutex> lock(ZmqMsgMutex);
    boost::unique_lock<boost::shared_mutex> lock(ZmqMsgMutex);
    std::string result = ZmqMsgBuffer;
    ZmqMsgBuffer.clear();
    //ZmqMsgBuffer = "system is running!";
    return result;
}

void removeSpaces(std::string& str) 
{
    // ʹ�ñ�׼��� remove_if �������ν����ɾ���ո�
    str.erase(std::remove_if(str.begin(), str.end(), [](char c) {
        return std::isspace(static_cast<unsigned char>(c));
        }), str.end());
}

const char* ConvertWideCharToMultiByte(const WCHAR* wideCharString)
{
    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wideCharString, -1, NULL, 0, NULL, NULL);
    char* multiByteString = new char[requiredSize];
    WideCharToMultiByte(CP_UTF8, 0, wideCharString, -1, multiByteString, requiredSize, NULL, NULL);
    return multiByteString;
}

bool KillProcessByName(const char* processName) 
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    bool found = false;
    char username[UNLEN + 1];
    DWORD usernameLen = UNLEN + 1;
    GetUserNameA(username, &usernameLen);

    // ����һ�����̿��գ���ȡϵͳ�����н��̵���Ϣ
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        LogError("Failed to create process snapshot.");
        return false;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    // �������̿��գ�����ָ���������͵�ǰ�û��Ľ���
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            // �ȽϽ��������û����Ƿ�ƥ��
            if (_stricmp(ConvertWideCharToMultiByte(pe32.szExeFile), processName) == 0) {
                found = true;
                // ��ȡ���̾��
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    // ��ֹ����
                    if (TerminateProcess(hProcess, 0)) {
                        //std::cout << "Process terminated successfully." << std::endl;
                    }
                    else {
                        LogError("Failed to terminate process.");
                        return false;
                    }
                    // �رս��̾��
                    CloseHandle(hProcess);
                }
                else {
                    LogError("Failed to open process.");
                    return false;
                }
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }

    // �رս��̿��վ��
    CloseHandle(hProcessSnap);

    if (!found) {
        //std::cout << "Process not found." << std::endl;
        return true;
    }

    return true;
}

// ��const char*���͵��ַ���ת��ΪLPCWSTR����
LPCWSTR ConvertToLPCWSTR(const char* str)
{
    std::vector<wchar_t> buffer;
    int length = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    buffer.resize(length);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, buffer.data(), length);
    return buffer.data();
}

bool StartProcessByPath(const std::string& processPath)
{
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));

    // ��������
    if (!CreateProcess(ConvertToLPCWSTR(processPath.c_str()),            // ��ִ���ļ�·��
        NULL,                  // �����в���������Ϊ�գ�
        NULL,                  // ���̾�����ɼ̳�
        NULL,                  // �߳̾�����ɼ̳�
        FALSE,                 // ���̳о��
        0,                     // Ĭ�ϴ�����־
        NULL,                  // ʹ�ø����̵Ļ�������
        NULL,                  // ʹ�ø����̵ĵ�ǰĿ¼
        &startupInfo,          // STARTUPINFOA �ṹ��
        &processInfo))         // PROCESS_INFORMATION �ṹ��
    {
        LogError1("Failed to start client: ", GetLastError());
        return false;
    }

    // �ȴ����̽���
    //WaitForSingleObject(processInfo.hProcess, INFINITE);

    // �رս��̺��߳̾��
    //CloseHandle(processInfo.hProcess);
    //CloseHandle(processInfo.hThread);

    return true;
}


int CreateDirectory(std::string path)
{
	size_t len = path.length();
	char tmpDirPath[256] = { 0 };
	for (size_t i = 0; i < len; i++)
	{
		tmpDirPath[i] = path[i];
		if (tmpDirPath[i] == '\\' || tmpDirPath[i] == '/')
		{
			if (_access(tmpDirPath, 0) == -1)
			{
				int ret = _mkdir(tmpDirPath);
				if (ret == -1) return ret;
			}
		}
	}
	return 0;
}

int getTime()
{
	return clock() / CLOCKS_PER_SEC;
}

std::string GetLocalIpAddressFromIniFile()
{
    std::string ip;
    std::string iniFilePath = "./CommConfig.ini";
    std::ifstream iniFile(iniFilePath);
    if (iniFile.is_open())
    {
        std::string line;
        std::regex ipRegex("Local\\s*=\\s*(\\d{1,3}\\.){3}\\d{1,3}");
        while (getline(iniFile, line))
        {
            std::smatch match;
            if (std::regex_search(line, match, ipRegex))
            {
                ip = match[0];
                ip = ip.substr(ip.find("=") + 1);
                break;
            }
        }
        iniFile.close();
    }
    return ip;
}

std::string GetRemoteIpAddressFromIniFile()
{
    std::string ip;
    std::string iniFilePath = "./CommConfig.ini";
    std::ifstream iniFile(iniFilePath);
    if (iniFile.is_open())
    {
        std::string line;
        std::regex ipRegex("Remote\\s*=\\s*(\\d{1,3}\\.){3}\\d{1,3}");
        while (getline(iniFile, line))
        {
            std::smatch match;
            if (std::regex_search(line, match, ipRegex))
            {
                ip = match[0];
                ip = ip.substr(ip.find("=") + 1);
                break;
            }
        }
        iniFile.close();
    }
    return ip;
}

int PrintAllNetAddrInfo()
{
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
    boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
    boost::asio::ip::tcp::resolver::iterator end; // End marker.

    std::cout << "List of ip : " << std::endl;
    while (iter != end)
    {
        boost::asio::ip::tcp::endpoint ep = *iter++;
        if (ep.address().is_v4())
        {
            std::cout << ep.address().to_string() << std::endl;
        }
    }
    return 0;
}

int SetTcpServerIpAddress(std::string& ip)
{
    std::cout << "Do you want to use the default IP address (" << ip << ") to start local server? (y/n)" << std::endl;
    char choice;
    std::cin >> choice;
    if (choice == 'n')
    {
        bool validIP = false;
        do
        {
            std::cout << "Please enter the custom IP address: ";
            std::cin >> ip;
            std::regex ipv4Regex("^([0-9]{1,3}\\.){3}[0-9]{1,3}$");
            if (std::regex_match(ip, ipv4Regex))
            {
                validIP = true;
            }
            else
            {
                std::cout << "Invalid IP address. Please enter a valid IPv4 address." << std::endl;
            }
        } while (!validIP);

        std::cout << "Custom IP address set to: " << ip << std::endl;

        // Update TcpServer.ini file with new IP address
        std::string iniFilePath = "./CommConfig.ini";
        std::ifstream iniFile(iniFilePath);
        if (iniFile.is_open())
        {
            std::string line;
            std::regex ipRegex("Local\\s*=\\s*(\\d{1,3}\\.){3}\\d{1,3}");
            std::string newIniContent;
            bool ipUpdated = false;
            while (getline(iniFile, line))
            {
                std::smatch match;
                if (std::regex_search(line, match, ipRegex))
                {
                    line = "Local = " + ip;
                    ipUpdated = true;
                }
                newIniContent += line + "\n";
            }
            iniFile.close();

            if (ipUpdated)
            {
                std::ofstream iniFile(iniFilePath);
                iniFile << newIniContent;
                iniFile.close();
            }
            else
            {
                LogError("Failed to update local server address in CommConfig.ini file.");
                return -1;
            }
        }
        else
        {
            LogError("Failed to open CommConfig.ini file.");
            return -1;
        }
    }
    return 0;
}

int SetZmqServerIpAddress(std::string& ip)
{
    std::cout << "Do you want to use the default IP address (" << ip << ") to connect the zmq server? (y/n)" << std::endl;
    char choice;
    std::cin >> choice;
    if (choice == 'n')
    {
        bool validIP = false;
        do
        {
            std::cout << "Please enter the custom IP address: ";
            std::cin >> ip;
            std::regex ipv4Regex("^([0-9]{1,3}\\.){3}[0-9]{1,3}$");
            if (std::regex_match(ip, ipv4Regex))
            {
                validIP = true;
            }
            else
            {
                std::cout << "Invalid IP address. Please enter a valid IPv4 address." << std::endl;
            }
        } while (!validIP);

        std::cout << "Custom IP address set to: " << ip << std::endl;

        // Update TcpServer.ini file with new IP address
        std::string iniFilePath = "./CommConfig.ini";
        std::ifstream iniFile(iniFilePath);
        if (iniFile.is_open())
        {
            std::string line;
            std::regex ipRegex("Remote\\s*=\\s*(\\d{1,3}\\.){3}\\d{1,3}");
            std::string newIniContent;
            bool ipUpdated = false;
            while (getline(iniFile, line))
            {
                std::smatch match;
                if (std::regex_search(line, match, ipRegex))
                {
                    line = "Remote = " + ip;
                    ipUpdated = true;
                }
                newIniContent += line + "\n";
            }
            iniFile.close();

            if (ipUpdated)
            {
                std::ofstream iniFile(iniFilePath);
                iniFile << newIniContent;
                iniFile.close();
            }
            else
            {
                LogError("Failed to update zmq server address in CommConfig.ini file.");
                return -1;
            }
        }
        else
        {
            LogError("Failed to open CommConfig.ini file.");
            return -1;
        }
    }
    return 0;
}
