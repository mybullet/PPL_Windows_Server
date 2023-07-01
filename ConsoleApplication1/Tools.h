#pragma once

#include <string>
#include <regex>
#include <fstream>
#include <boost/process.hpp>
#include <Windows.h>
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <tlhelp32.h>
#include <Lmcons.h>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <openssl/rand.h>
#include <openssl/evp.h>

#define LENGTH_SECRET_KEY 32

extern std::string ZmqMsgBuffer;
extern boost::shared_mutex ZmqMsgMutex;
extern std::string SYSTEM_IS_RUNNING;

//path必须要用双反斜杠结尾，或者单斜岗
int CreateDirectory(std::string path);
int getTime();
const char* ConvertWideCharToMultiByte(const WCHAR* wideCharString);
bool KillProcessByName(const char* processName);
LPCWSTR ConvertToLPCWSTR(const char* str);
bool StartProcessByPath(const std::string& processPath);
std::string GetLocalIpAddressFromIniFile();
int PrintAllNetAddrInfo();
std::string GetRemoteIpAddressFromIniFile();
int SetTcpServerIpAddress(std::string& ip);
int SetZmqServerIpAddress(std::string& ip);
void removeSpaces(std::string& str);
void UpdateZmqMessage(std::string msg);
std::string GetZmqMessage();
std::string ConvertSpdStr2Boost(const std::string& ss);
void MainThread(std::string zmqAddress);
std::string encrypt(const std::string& message, const std::string& key);
std::string decrypt(const std::string& encryptedMessage, const std::string& key);