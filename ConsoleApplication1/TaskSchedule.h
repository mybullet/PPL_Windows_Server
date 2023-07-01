#pragma once
#include <map>
#include <utility>
#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <thread>
#include <list>
#include "Logger.h"
enum eTaskState
{
	Error = -1,
	NewAdd = 0,
	Processing = 1,
	Processed = 2
};

class TaskSchedule
{
public:
	void AddTask(std::string);
	std::string GetTask();
	void SubmitTask(std::tuple<std::string, std::string>);
	std::string GetResult(std::string);
	std::tuple<std::string, eTaskState, std::string> SaveRecord();
	void CloseTask(std::string);

public:
	std::map<std::string, std::pair<eTaskState, std::string> > m_TaskAccpetMap;
	boost::shared_mutex m_AccpetMutex;

	std::list<std::tuple<std::string, eTaskState, std::string> > m_TaskProcessList;  
	boost::shared_mutex m_ProcessMutex;

};

