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
    // 创建加密上下文
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, reinterpret_cast<const unsigned char*>(key.c_str()), NULL);

    // 加密数据
    int cipherLen = message.length() + EVP_MAX_BLOCK_LENGTH;
    unsigned char* cipherText = new unsigned char[cipherLen];
    int len;
    EVP_EncryptUpdate(ctx, cipherText, &len, reinterpret_cast<const unsigned char*>(message.c_str()), message.length());
    cipherLen = len;
    EVP_EncryptFinal_ex(ctx, cipherText + len, &len);
    cipherLen += len;

    // 清理并释放资源
    EVP_CIPHER_CTX_free(ctx);

    // 返回加密后的数据
    std::string encryptedText(reinterpret_cast<char*>(cipherText), cipherLen);
    delete[] cipherText;
    return encryptedText;
}

std::string decrypt(const std::string& encryptedMessage, const std::string& key) 
{
    // 创建解密上下文
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, reinterpret_cast<const unsigned char*>(key.c_str()), NULL);

    // 解密数据
    int plainLen = encryptedMessage.length();
    unsigned char* plainText = new unsigned char[plainLen];
    int len;
    EVP_DecryptUpdate(ctx, plainText, &len, reinterpret_cast<const unsigned char*>(encryptedMessage.c_str()), encryptedMessage.length());
    plainLen = len;
    EVP_DecryptFinal_ex(ctx, plainText + len, &len);
    plainLen += len;

    // 清理并释放资源
    EVP_CIPHER_CTX_free(ctx);

    // 返回解密后的数据
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

    int timeout = 30;  // 连续无响应的超时时间（秒）
    int counter = 0;   // 计数器，记录连续无响应的时间
    bool zmqRunning = true;
    bool needGetSecretKey = true;
    std::string secretKey = "";
    int wait_time = 500;

    while (zmqRunning)
    {
        try
        {
            // 判断是否连接到服务端
            if (socket.handle() == nullptr)
            {
                //std::cout << "reconnecting......." << std::endl;
                socket = zmq::socket_t(context, zmq::socket_type::req);  // 创建新的 socket 对象
                socket.connect(zmqCommStr.c_str());  // 建立新的连接
                //std::cout << "Reconnected to the server" << std::endl;
            }

            std::string message;
            if (needGetSecretKey)
            {
                message = "GET@";

                zmq::message_t request(message.size());
                memcpy(request.data(), message.data(), message.size());

                // 发送请求给服务端
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

                // 发送请求给服务端
                socket.send(request, zmq::send_flags::none);
            }
            
            
            //std::cout << "Success to send hello server!" << std::endl;

            // 等待并接收服务端的回复，最多等待500毫秒
            zmq::pollitem_t items[] = { { socket, 0, ZMQ_POLLIN, 0 } };
            zmq::poll(items, 1, std::chrono::milliseconds(500));

            if (items[0].revents & ZMQ_POLLIN)
            {
                zmq::message_t reply;
                socket.recv(reply, zmq::recv_flags::none);
                std::string response(static_cast<char*>(reply.data()), reply.size());

                if (needGetSecretKey)
                {
                    // 检查回复消息是否带有 "SECRET" 前缀
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

                    counter = 0;  // 重置计数器
                }
                else
                {
                    std::cout << "Received origin reply from server: " << response << std::endl;
                    response = decrypt(response, secretKey);

                    // 检查回复消息是否带有 "REMOTE" 前缀
                    if (response.find("REMOTE@") != 0)
                    {
                        std::cout << "received invalid reply from server: " << response << std::endl;
                        counter++;  // 计数器累加
                    }
                    else
                    {
                        std::cout << "Received reply from server: " << response << std::endl;

                        counter = 0;  // 重置计数器
                    }
                }

                
            }
            else
            {
                std::cout << "receiving is timeout!" << std::endl;
                counter++;  // 计数器累加
            }

            if (0 != counter)
            {
                std::cout << "Counter is: " << counter << std::endl;
            }

            // 判断是否达到超时时间
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
            socket.close();  // 关闭之前的连接
            secretKey.clear();
            needGetSecretKey = true;
            counter++;  // 计数器累加

            if (0 != counter)
            {
                std::cout << "Counter is: " << counter << std::endl;
            }

            // 判断是否达到超时时间
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

// 定义槽函数
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
    // 使用标准库的 remove_if 函数结合谓词来删除空格
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

    // 创建一个进程快照，获取系统中所有进程的信息
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        LogError("Failed to create process snapshot.");
        return false;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    // 遍历进程快照，查找指定进程名和当前用户的进程
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            // 比较进程名和用户名是否匹配
            if (_stricmp(ConvertWideCharToMultiByte(pe32.szExeFile), processName) == 0) {
                found = true;
                // 获取进程句柄
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    // 终止进程
                    if (TerminateProcess(hProcess, 0)) {
                        //std::cout << "Process terminated successfully." << std::endl;
                    }
                    else {
                        LogError("Failed to terminate process.");
                        return false;
                    }
                    // 关闭进程句柄
                    CloseHandle(hProcess);
                }
                else {
                    LogError("Failed to open process.");
                    return false;
                }
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }

    // 关闭进程快照句柄
    CloseHandle(hProcessSnap);

    if (!found) {
        //std::cout << "Process not found." << std::endl;
        return true;
    }

    return true;
}

// 将const char*类型的字符串转换为LPCWSTR类型
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

    // 启动进程
    if (!CreateProcess(ConvertToLPCWSTR(processPath.c_str()),            // 可执行文件路径
        NULL,                  // 命令行参数（这里为空）
        NULL,                  // 进程句柄不可继承
        NULL,                  // 线程句柄不可继承
        FALSE,                 // 不继承句柄
        0,                     // 默认创建标志
        NULL,                  // 使用父进程的环境变量
        NULL,                  // 使用父进程的当前目录
        &startupInfo,          // STARTUPINFOA 结构体
        &processInfo))         // PROCESS_INFORMATION 结构体
    {
        LogError1("Failed to start client: ", GetLastError());
        return false;
    }

    // 等待进程结束
    //WaitForSingleObject(processInfo.hProcess, INFINITE);

    // 关闭进程和线程句柄
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
