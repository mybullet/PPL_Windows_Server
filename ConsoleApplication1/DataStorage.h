#pragma once
#ifndef _DATA_STORAGE_
#define _DATA_STORAGE_

#include "TaskSchedule.h"
#include "SqliteWrapper.hpp"
#include "Logger.h"
extern TaskSchedule g_scheduler;


class DataStorage
{
public:
	explicit DataStorage(bool isRun);
	virtual ~DataStorage();
	void Start();
	void Stop();

	void Insert(std::tuple<std::string, std::string, std::string> sourceData);
	void DeleteByDT(std::string dataTime);
	void DeleteByName(std::string taskName);
	void Update(std::tuple<std::string, std::string, std::string> sourceData);
	std::list<std::tuple<std::string, std::string, std::string> > SelectByDT(std::string dataTime);
	std::list<std::tuple<std::string, std::string, std::string> > SelectByName(std::string taskName);
	void PrintAll();


private:
	 //Creates a SQLite connection object
	static Sqlite::SqliteConnection connection;
	bool m_running;
};

#endif // !_DATA_STORAGE_



