#pragma once
#ifndef _OCR_PROCESS_
#define _OCR_PROCESS_

#include <python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <boost/python.hpp>
#include <boost/thread/thread.hpp>
#include "TaskSchedule.h"
#include "Logger.h"
#define BOOST_ALL_DYN_LINK 1 //  BOOST_ALL_DYN_LINK
extern TaskSchedule g_scheduler;

//全局解释和线程锁
class PyGILThreadLock
{
public:
	PyGILThreadLock()
	{
		_save = NULL;
		nStatus = 0;
		nStatus = PyGILState_Check();   //检测当前线程是否拥有GIL  
		PyGILState_STATE gstate;
		if (!nStatus)
		{
			gstate = PyGILState_Ensure();   //如果没有GIL，则申请获取GIL 
			nStatus = 1;
		}
		_save = PyEval_SaveThread();
		PyEval_RestoreThread(_save);
	}
	~PyGILThreadLock()
	{
		_save = PyEval_SaveThread();
		PyEval_RestoreThread(_save); 
		if (nStatus)
		{
			PyGILState_Release(gstate);    //释放当前线程的GIL  
		}
	}

	static int m_ocrThreadNumber;
	static int m_run;
private:
	PyGILState_STATE gstate;
	PyThreadState* _save;
	int nStatus;
};



void StartOcrProcess();
void InvokeFuncByBoost(); //需要禁止外面调用
void StopOcrProcess();

#endif // !_OCR_PROCESS_


