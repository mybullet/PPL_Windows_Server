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

// 定义信号类型, 创建信号对象
extern boost::signals2::signal<void(std::string)> ZmqSignal;


//如果出现乱序，则需要加锁
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


// 自定义日志处理程序
//template <typename Mutex>
//class MySink : public spdlog::sinks::sink
//{
//public:
//	void log(const spdlog::details::log_msg& msg) override
//	{
//		// 获取日志消息文本
//		std::string logMsg = this->formatter_->format(msg);
//
//		// 发射信号
//		ZmqSignal(logMsg);
//
//		// 打印日志消息
//		std::cout << logMsg << std::endl;
//	}
//
//	void flush() override {}
//};



#endif // !__LOGGER__



