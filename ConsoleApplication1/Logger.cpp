#include "Logger.h"
//#include <iostream>

bool Logger::TEST_CMD = false;

//std::shared_ptr<spd::logger> Logger::m_consoleLogger = spd::stdout_color_mt("console");
//std::shared_ptr<spd::logger> Logger::m_fileLogger = spd::rotating_logger_mt("async_file_logger", "./DailyLog/Rotating_Log.txt", 1024 * 20, 10);
std::shared_ptr<spd::logger> Logger::m_consoleLogger = NULL;
std::shared_ptr<spd::logger> Logger::m_fileLogger = NULL;

bool InitialGlobalLogger(bool test)
{
	Logger::TEST_CMD = test;
	return Logger::InitLogger();
}
void DestoryGlobalLogger()
{
	Logger::DestoryLogger();
}


bool Logger::InitLogger()
{
	try
	{
		spd::set_pattern("[%l] [%Y/%m/%d %X.%e] [TID:%t] %v");
		spd::set_level(spd::level::trace);
		// Just call spdlog::set_async_mode(q_size) and all created loggers from now on will be asynchronous..
		size_t q_size = 1024; //queue size must be power of 2
		spdlog::set_async_mode(q_size);
		
		m_consoleLogger = spd::stdout_color_mt("console");
		m_fileLogger = spd::rotating_logger_mt("async_file_logger", "./DailyLog/Rotating_Log.txt", 1024 * 20, 10);

		// 连接信号和槽
		ZmqSignal.connect(&UpdateZmqMessage);


		return true;
	}
	// Exceptions will only be thrown upon failed logger or sink construction (not during logging)
	catch (const spd::spdlog_ex& ex)
	{
		//std::cout << "Log init failed: " << ex.what() << std::endl;
		return false;
	}
}

void Logger::DestoryLogger()
{
	Logger::m_consoleLogger->info("Release and close all loggers!");
	Logger::m_fileLogger->info("Release and close all loggers!");
	spdlog::drop_all();
}


