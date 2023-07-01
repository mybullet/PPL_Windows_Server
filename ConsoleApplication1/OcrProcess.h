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

//ȫ�ֽ��ͺ��߳���
class PyGILThreadLock
{
public:
	PyGILThreadLock()
	{
		_save = NULL;
		nStatus = 0;
		nStatus = PyGILState_Check();   //��⵱ǰ�߳��Ƿ�ӵ��GIL  
		PyGILState_STATE gstate;
		if (!nStatus)
		{
			gstate = PyGILState_Ensure();   //���û��GIL���������ȡGIL 
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
			PyGILState_Release(gstate);    //�ͷŵ�ǰ�̵߳�GIL  
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
void InvokeFuncByBoost(); //��Ҫ��ֹ�������
void StopOcrProcess();

#endif // !_OCR_PROCESS_


