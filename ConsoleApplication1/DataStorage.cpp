#include "DataStorage.h"
#include <iostream>
#include <iomanip>
#include <time.h>

Sqlite::SqliteConnection DataStorage::connection = Sqlite::SqliteConnection("MyProfile.db");

DataStorage::DataStorage(bool isRun): m_running(isRun)
{
    try {
        //Creates a in-memory database
        //const Sqlite::SqliteConnection connection = Sqlite::SqliteConnection::memory();

        sqliteExecute(connection, "create table IF NOT EXISTS OcrProcessRecord ("
            "CompleteTime text NOT NULL , "
            "TaskName text NOT NULL ,"
			"OcrResult text "
            ")");

        
    }
    catch (const Sqlite::exception& e) {
        std::cout << e.errorMessage_ << '(' << e.errorCode_ << ')';
    }
}



DataStorage::~DataStorage()
{
	m_running = false;
}

#define _CRT_SECURE_NO_WARNINGS
void DataStorage::Start()
{
	//std::cout << "DataStorage " << std::this_thread::get_id() << std::endl;
	LogInfo("start data storage task");
	char tt[20];
	while (m_running)
	{
		//std::cout << "DataStorage.size: " << g_scheduler.m_TaskProcessList.size() << std::endl;
		std::tuple<std::string, eTaskState, std::string> ret = g_scheduler.SaveRecord();
		std::string taskName, processResult;
		eTaskState state;
		std::tie(taskName, state, processResult) = ret;
		if (state == eTaskState::Processed)
		{
			//std::cout << "DataStorage : " << taskName << " + " << state << " + " << processResult << std::endl;
			//std::cout << "DataStorage.size: " << g_scheduler.m_TaskProcessList.size() << std::endl;
			time_t timep;
			tm p;
			time(&timep); //获取1900年1月1日0点0分0秒到现在经过的秒数
			localtime_s(&p, &timep); //将秒数转换为本地时间,年从1900算起,需要+1900,月为0-11,所以要+1
			memset(tt, 0, sizeof(tt));
			sprintf_s(tt, "%d/%d/%d %02d:%02d:%02d", 1900 + p.tm_year, 1 + p.tm_mon, p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec);
			Insert({ tt , taskName , processResult });
			//PrintAll();
		}
		else
		{
			//std::cout << "there is no processed task, please wait! " << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	LogInfo("close data storage task");
}


void DataStorage::Stop()
{
	m_running = false;
}

void DataStorage::Insert(std::tuple<std::string, std::string, std::string> sourceData)
{
	try
	{
		std::string time, task, result;
		std::tie(time, task, result) = sourceData;
		//Creates a SQLite statment and binds the values in it
		Sqlite::SqliteStatement statement(connection, "insert into OcrProcessRecord(CompleteTime, TaskName, OcrResult ) values (?, ?, ?)", time, task, result);
		//Executes SqliteStatement
		statement.execute();
	}	
	catch (const Sqlite::exception& e) {
		std::cout << e.errorMessage_ << '(' << e.errorCode_ << ')';
	}
}


void DataStorage::DeleteByDT(std::string dataTime)
{
	try
	{
		std::string sql = "DELETE FROM OcrProcessRecord WHERE CompleteTime = '" + dataTime + "'";
		//Creates a SQLite statment and binds the values in it
		Sqlite::SqliteStatement statement(connection, sql.c_str());
		//Executes SqliteStatement
		statement.execute();
	}
	catch (const Sqlite::exception& e) {
		std::cout << e.errorMessage_ << '(' << e.errorCode_ << ')';
	}
}
void DataStorage::DeleteByName(std::string taskName)
{
	try
	{
		std::string sql = "DELETE FROM OcrProcessRecord WHERE TaskName = '" + taskName + "'";
		//Creates a SQLite statment and binds the values in it
		Sqlite::SqliteStatement statement(connection, sql.c_str());
		//Executes SqliteStatement
		statement.execute();
	}
	catch (const Sqlite::exception& e) {
		std::cout << e.errorMessage_ << '(' << e.errorCode_ << ')';
	}
}

void DataStorage::Update(std::tuple<std::string, std::string, std::string> sourceData)
{
	try
	{
		std::string time, task, result;
		std::tie(time, task, result) = sourceData;
		std::string sql;
		if (time.empty())
		{
			sql = "UPDATE OcrProcessRecord SET OcrResult = '" + result + "' WHERE TaskName = '" + task + "'";
			
		}
		else
		{
			sql = "UPDATE OcrProcessRecord SET OcrResult = '" + result + "' WHERE TaskName = '" + task + "' AND CompleteTime = '" + time + "'";
			
		}

		//Creates a SQLite statment and binds the values in it
		Sqlite::SqliteStatement statement(connection, sql.c_str());
		//Executes SqliteStatement
		statement.execute();
	}
	catch (const Sqlite::exception& e) {
		std::cout << e.errorMessage_ << '(' << e.errorCode_ << ')';
	}
}
std::list<std::tuple<std::string, std::string, std::string> > DataStorage::SelectByDT(std::string dataTime)
{
	try 
	{
		std::list<std::tuple<std::string, std::string, std::string> > ret;
		std::string sql = "select CompleteTime, TaskName, OcrResult from OcrProcessRecord WHERE CompleteTime = '" + dataTime + "'";
		for (auto row : Sqlite::SqliteStatement(connection, sql.c_str())) 
		{
			std::tuple<std::string, std::string, std::string> item = { row.getString(0), row.getString(1), row.getString(2) };
			ret.push_back(item);
			//std::cout << std::left << std::setw(8) << row.getString() << " : " << std::right << std::setw(6) << row.getInt(1) << std::endl;
			//std::cout << row.getString(0) << " : " << row.getString(1) << " : " << row.getString(2) << std::endl;
		}
		return ret;
	}
	catch (const Sqlite::exception& e) {
		std::cout << e.errorMessage_ << '(' << e.errorCode_ << ')';
	}
}
std::list<std::tuple<std::string, std::string, std::string> > DataStorage::SelectByName(std::string taskName)
{
	try
	{
		std::list<std::tuple<std::string, std::string, std::string> > ret;
		std::string sql = "select CompleteTime, TaskName, OcrResult from OcrProcessRecord WHERE TaskName = '" + taskName + "'";
		for (auto row : Sqlite::SqliteStatement(connection, sql.c_str()))
		{
			std::tuple<std::string, std::string, std::string> item = { row.getString(0), row.getString(1), row.getString(2) };
			ret.push_back(item);
			//std::cout << std::left << std::setw(8) << row.getString() << " : " << std::right << std::setw(6) << row.getInt(1) << std::endl;
			//std::cout << row.getString(0) << " : " << row.getString(1) << " : " << row.getString(2) << std::endl;
		}
		return ret;
	}
	catch (const Sqlite::exception& e) {
		std::cout << e.errorMessage_ << '(' << e.errorCode_ << ')';
	}
}
void DataStorage::PrintAll()
{
	std::cout << "PrintAll : " << std::endl;
	for (auto row : Sqlite::SqliteStatement(connection, "select CompleteTime, TaskName, OcrResult from OcrProcessRecord ORDER BY CompleteTime DESC")) 
	{
		std::cout << row.getString(0) << " : " << row.getString(1) << " : " << row.getString(2) << std::endl;
	}
}