#pragma once
#ifndef __LOGGER__
#define __LOGGER__

#include "spdlog/spdlog.h"
#include <spdlog/sinks/base_sink.h>
//#include <boost/thread/mutex.hpp>
//#include <boost/thread/thread.hpp>
#include "Tools.h"
#include <boost/signals2.hpp>
#include <boost/format.hpp>

// �����ź�����, �����źŶ���
extern boost::signals2::signal<void(std::string)> ZmqSignal;


//���������������Ҫ����
//static boost::shared_mutex m_mutex;
//boost::unique_lock<boost::shared_mutex> lock(m_mutex);



namespace spd = spdlog;

#define LogInfo(message) \
spd::apply_all([&](std::shared_ptr<spdlog::logger> allLoggers) { if (Logger::TEST_CMD) allLoggers->info("File:{} Line:{}",__FILE__,__LINE__); \
allLoggers->info(message); ZmqSignal(message); }) 
#define LogInfo1(message,arg1) \
spd::apply_all([&](std::shared_ptr<spdlog::logger> allLoggers) { if (Logger::TEST_CMD) allLoggers->info("File:{} Line:{}",__FILE__,__LINE__); \
allLoggers->info(message,arg1); ZmqSignal(boost::str(boost::format(ConvertSpdStr2Boost(message)) % arg1)); })
#define LogInfo2(message,arg1,arg2) \
spd::apply_all([&](std::shared_ptr<spdlog::logger> allLoggers) { if (Logger::TEST_CMD) allLoggers->info("File:{} Line:{}",__FILE__,__LINE__); \
allLoggers->info(message,arg1,arg2); ZmqSignal(boost::str(boost::format(ConvertSpdStr2Boost(message)) % arg1 % arg2)); })
#define LogInfo3(message,arg1,arg2,arg3) \
spd::apply_all([&](std::shared_ptr<spdlog::logger> allLoggers) { if (Logger::TEST_CMD) allLoggers->info("File:{} Line:{}",__FILE__,__LINE__); \
allLoggers->info(message,arg1,arg2,arg3); ZmqSignal(boost::str(boost::format(ConvertSpdStr2Boost(message)) % arg1 % arg2 % arg3)); })


#define LogError(message) \
spd::apply_all([&](std::shared_ptr<spdlog::logger> allLoggers) { if (Logger::TEST_CMD) allLoggers->info("File:{} Line:{}",__FILE__,__LINE__); \
allLoggers->error(message); ZmqSignal(message); }) 
#define LogError1(message,arg1) \
spd::apply_all([&](std::shared_ptr<spdlog::logger> allLoggers) { if (Logger::TEST_CMD) allLoggers->info("File:{} Line:{}",__FILE__,__LINE__); \
allLoggers->error(message,arg1); ZmqSignal(boost::str(boost::format(ConvertSpdStr2Boost(message)) % arg1)); })
#define LogError2(message,arg1,arg2) \
spd::apply_all([&](std::shared_ptr<spdlog::logger> allLoggers) { if (Logger::TEST_CMD) allLoggers->info("File:{} Line:{}",__FILE__,__LINE__); \
allLoggers->error(message,arg1,arg2); ZmqSignal(boost::str(boost::format(ConvertSpdStr2Boost(message)) % arg1 % arg2)); })
#define LogError3(message,arg1,arg2,arg3) \
spd::apply_all([&](std::shared_ptr<spdlog::logger> allLoggers) { if (Logger::TEST_CMD) allLoggers->info("File:{} Line:{}",__FILE__,__LINE__); \
allLoggers->error(message,arg1,arg2,arg3); ZmqSignal(boost::str(boost::format(ConvertSpdStr2Boost(message)) % arg1 % arg2 % arg3)); })


bool InitialGlobalLogger(bool testMode);
void DestoryGlobalLogger();

class Logger
{
	friend bool InitialGlobalLogger(bool testMode);
	friend void DestoryGlobalLogger();

private:
	/*Logger();
	~Logger();*/

	static bool InitLogger();
	static void DestoryLogger();

private:

	static std::shared_ptr<spd::logger>  m_consoleLogger;
	static std::shared_ptr<spd::logger>  m_fileLogger;

public:
	static bool TEST_CMD;
	
};


// �Զ�����־�������
//template <typename Mutex>
//class MySink : public spdlog::sinks::sink
//{
//public:
//	void log(const spdlog::details::log_msg& msg) override
//	{
//		// ��ȡ��־��Ϣ�ı�
//		std::string logMsg = this->formatter_->format(msg);
//
//		// �����ź�
//		ZmqSignal(logMsg);
//
//		// ��ӡ��־��Ϣ
//		std::cout << logMsg << std::endl;
//	}
//
//	void flush() override {}
//};



#endif // !__LOGGER__



