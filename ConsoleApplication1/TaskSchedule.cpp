#include "TaskSchedule.h"
#include <iostream>

void TaskSchedule::AddTask(std::string taskName)
{
	if (taskName.empty())
		return;

	{
		boost::unique_lock<boost::shared_mutex> lock(m_AccpetMutex);
		std::map<std::string, std::pair<eTaskState, std::string> >::iterator it = m_TaskAccpetMap.begin();
		for (; it != m_TaskAccpetMap.end(); it++)
		{
			if (it->first == taskName)
			{
				//std::cout << "任务已在队列,不再加入！" << std::endl;
				return;
			}
		}
	}

	{
		boost::unique_lock<boost::shared_mutex> lock(m_AccpetMutex);
		std::pair<eTaskState, std::string> result = { eTaskState::NewAdd, ""};
		m_TaskAccpetMap.insert({ taskName, result });
	}

	{	
		boost::unique_lock<boost::shared_mutex> lock(m_ProcessMutex);
		std::tuple<std::string, eTaskState, std::string> process(taskName, eTaskState::NewAdd, "");
		m_TaskProcessList.push_back(process);
	}
	
}

std::string TaskSchedule::GetTask()
{
	std::string ret;
	ret.clear();

	{
		boost::unique_lock<boost::shared_mutex> lock(m_ProcessMutex);
		std::list<std::tuple<std::string, eTaskState, std::string> >::iterator it = m_TaskProcessList.begin();

		for (; it != m_TaskProcessList.end(); it++)
		{
			if (std::get<1>(*it) == eTaskState::NewAdd)
			{
				ret = std::get<0>(*it);
				std::get<1>(*it) = eTaskState::Processing;
				break;
			}
		}
	}

	{
		boost::unique_lock<boost::shared_mutex> lock(m_AccpetMutex);
		std::map<std::string, std::pair<eTaskState, std::string> >::iterator it = m_TaskAccpetMap.begin();
		for (; it != m_TaskAccpetMap.end(); it++)
		{
			if (it->first == ret && it->second.first == eTaskState::NewAdd)
			{
				it->second.first = eTaskState::Processing;
				break;
			}
		}
	}
	
	return ret;
}

void TaskSchedule::SubmitTask(std::tuple<std::string, std::string> processedTask)
{
	std::string taskName, processResult;
	std::tie(taskName, processResult) = processedTask;
	
	{
		boost::unique_lock<boost::shared_mutex> lock(m_AccpetMutex);
		std::map<std::string, std::pair<eTaskState, std::string> >::iterator it = m_TaskAccpetMap.begin();
		for (; it != m_TaskAccpetMap.end(); it++)
		{
			//std::cout << "outoutoutoutoutoutoutoutout : " << it->first << "  " << it->second.first << "   " << it->second.second << std::endl; 
			if (it->first == taskName && it->second.first == eTaskState::Processing)
			{				
				it->second.second = processResult;
				it->second.first = eTaskState::Processed;
				//std::cout << "submit success!!!!!!!!" << std::endl;
				break;
			}
		}
	}

	{
		boost::unique_lock<boost::shared_mutex> lock(m_ProcessMutex);
		std::list<std::tuple<std::string, eTaskState, std::string> >::iterator it = m_TaskProcessList.begin();

		for (; it != m_TaskProcessList.end(); it++)
		{
			if (std::get<1>(*it) == eTaskState::Processing && std::get<0>(*it) == taskName)
			{
				std::get<2>(*it) = processResult;
				std::get<1>(*it) = eTaskState::Processed;
				//std::cout << "submit success!!!???????!!" << std::endl;
				break;
			}
		}
	}
}

std::tuple<std::string, eTaskState, std::string> TaskSchedule::SaveRecord()
{
	std::tuple<std::string, eTaskState, std::string> ret;
	
	{
		boost::unique_lock<boost::shared_mutex> lock(m_ProcessMutex);
		std::list<std::tuple<std::string, eTaskState, std::string> >::iterator it = m_TaskProcessList.begin();

		for (; it != m_TaskProcessList.end(); it++)
		{
			if (std::get<1>(*it) == eTaskState::Processed)
			{
				ret = *it;
				m_TaskProcessList.erase(it);
				break;
			}
		}
	}
	return ret;
}

std::string TaskSchedule::GetResult(std::string taskName)
{
	std::string ret;
	if (taskName == "")
	{
		ret = "";
		//std::cout << "111" << std::endl;
	}
	else
	{
		boost::shared_lock<boost::shared_mutex> lock(m_AccpetMutex);
		std::map<std::string, std::pair<eTaskState, std::string> >::iterator it = m_TaskAccpetMap.begin();
		for (; it != m_TaskAccpetMap.end(); it++)
		{
			if (it->first == taskName && it->second.first == eTaskState::Processed)
			{
				ret = it->second.second;
				break;
			}
			//std::cout << "222" << std::endl;
		}
		//std::cout << "333" << std::endl;
	}
	return ret;
}

void TaskSchedule::CloseTask(std::string taskName)
{
	boost::unique_lock<boost::shared_mutex> lock(m_AccpetMutex);
	std::map<std::string, std::pair<eTaskState, std::string> >::iterator it = m_TaskAccpetMap.begin();
	for (; it != m_TaskAccpetMap.end(); it++)
	{
		if (it->first == taskName && it->second.first == eTaskState::Processed)
		{
			m_TaskAccpetMap.erase(it);
			//std::cout << "TaskSchedule::CloseTask" << std::endl;
			break;
		}
	}
}
