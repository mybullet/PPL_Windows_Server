#include "OcrProcess.h"
#include <iostream>

boost::thread_group ocrThreadGroup;
int PyGILThreadLock::m_ocrThreadNumber = 1;
int PyGILThreadLock::m_run = true;
void StartOcrProcess()
{
	//Py_SetPythonHome(L"x:\\python37");
	Py_Initialize(); //��ʼ��Python����
	if (!Py_IsInitialized()) //����Ƿ��ʼ���ɹ�
	{
		LogError("fail to init Python env");
		return;
	}
	else
	{
		PyEval_InitThreads();     //�������߳�֧��
		int nInit = PyEval_ThreadsInitialized();  //����߳�֧���Ƿ����ɹ�
		if (nInit)
		{
			PyEval_SaveThread();  //��Ϊ����PyEval_InitThreads�ɹ��󣬵�ǰ�߳̾�ӵ����GIL���ͷŵ�ǰ�̵߳�GIL��
		}
		LogInfo("success to init Python env");
	}

	for (int i = 0; i < PyGILThreadLock::m_ocrThreadNumber; i++)
	{
		ocrThreadGroup.create_thread(InvokeFuncByBoost);
	}

	LogInfo1("create {} task to execute ocr", PyGILThreadLock::m_ocrThreadNumber);
}

void StopOcrProcess()
{
	PyGILThreadLock::m_run = false;
	ocrThreadGroup.join_all();                 //Wait for all threads to finish
	//PyGILThreadLock lock;
	//Py_Finalize();
}


void InvokeFuncByBoost()
{
	std::vector<std::string> paras;
	while (PyGILThreadLock::m_run)
	{
		paras.clear();
		std::string taskName = g_scheduler.GetTask();
		if (taskName.empty())
		{
			std::this_thread::sleep_for(std::chrono::seconds(5));
			continue;
		}
		else
		{
			paras.push_back(taskName);
		}
		
		try
		{
			PyGILThreadLock lock;
			PyRun_SimpleString("import sys\n");
			PyRun_SimpleString("sys.path.append('./Python_Scripts')");

			boost::python::object pymodule(boost::python::import("scan"));
			boost::python::object pyresult(pymodule.attr("convertImage2String")(boost::python::object(paras[0])/*, boost::python::object(paras[1])*/));
			//std::cout << "ocr result : " << boost::python::extract<char*>(pyresult) << " by boost::python" << std::endl;

			std::string result = std::string(boost::python::extract<char*>(pyresult));
			std::tuple<std::string, std::string> processedResult = { taskName , result };

			g_scheduler.SubmitTask(processedResult);

			LogInfo("finish an ocr task");
		}
		catch (...)
		{
			PyErr_Print();
			PyErr_Clear();
			Py_Finalize();

		}

	}

}


