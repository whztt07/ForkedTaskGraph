#include "YTaskGraph.h"
#include <iostream>
#include <assert.h>
#include "Runnable.h"
#include "RunnableThread.h"
#include "Event.h"
#include <algorithm>
#include <cstdlib>

float GetRundomTime()
{
	return (float)(std::rand()) / 1000000.0f;
}
ThreadSafeLockValueArray<int> GComputeResultTest;
class YTestJob : public YJob
{
public:
	YTestJob(int n)
		:JobID(n)
	{

	}
	virtual void Task(int nThreadID)
	{
		//std::cout << "Job ID is " << JobID  << "thread ID "<< nThreadID<< std::endl;
		//FPlatformProcess::Sleep(GetRundomTime());
		GComputeResultTest.Push(JobID);
	}
	int JobID;
};


class YTaskGraphThread : public FRunnableThread
{
public:
	friend class YTaskGraphThreadProc;
	virtual void SetThreadPriority(EThreadPriority NewPriority) override
	{

	}

	virtual void Suspend(bool bShouldPause) override
	{

	}

	virtual bool Kill(bool bShouldWait = true) override
	{

	}
	virtual void WaitForCompletion() override
	{

	}
};

class YTaskGraphThreadProc :public FRunnable
{
public:
	YTaskGraphThreadProc()
	{
		EventWaitforIdle = FPlatformProcess::GetSynchEventFromPool(false);
	}

	virtual unsigned int Run() override
	{
		while (1)
		{
			YJob* JobToDo = JobArray.Pop();
			if (JobToDo)
			{
				FRunnableThread * Thread = FRunnableThread::GetRunnableThread();
				int ThreadID = Thread->GetThreadID();
				JobToDo->Task(ThreadID);
				JobToDo->~YJob();
				delete JobToDo;
			}
			else
			{
				EventWaitforIdle->Wait(20000);
			}
		}
	}
	void AddJob(YJob* InJob)
	{
		int JobsToDoNum = JobArray.Num();
		JobArray.Push(InJob);
		if (JobsToDoNum == 0)
		{
			EventWaitforIdle->Trigger();
		}
	}
	ThreadSafeLockPointerArray<YJob> JobArray;
	FEvent*					EventWaitforIdle;
};

void YTaskGraphTest()
{
	FRunnableThread* pThread[10];
	YTaskGraphThreadProc* ThreadProc[10];
	for (int i = 0; i < 10; ++i)
	{
		ThreadProc[i] = new YTaskGraphThreadProc();
	}
	const int nThreadNum = 10;
	for (int i = 0; i < nThreadNum; ++i)
	{
		pThread[i] = YTaskGraphThread::Create(ThreadProc[i], TEXT("AnyThread"));
	}

	auto TestFunc = [&](int nTestNumCount) 
	{
		GComputeResultTest.InternalArray.clear();
		for (int i = 0; i < nTestNumCount; ++i)
		{
			ThreadProc[i % nThreadNum]->AddJob(new YTestJob(i));
			//FPlatformProcess::Sleep(GetRundomTime());
		}
		//FPlatformProcess::Sleep(5);
		while (1)
		{
			if (GComputeResultTest.Num() == nTestNumCount)
			{
				std::sort(GComputeResultTest.InternalArray.begin(), GComputeResultTest.InternalArray.end());
				bool bResultCorrect = true;
				for (int i = 0; i < nTestNumCount; ++i)
				{
					if (GComputeResultTest.InternalArray[i] != i)
					{
						bResultCorrect = false;
						break;
					}
				}
				if (bResultCorrect)
				{
					std::cout << " YTaskGraphCorrect " << std::endl;
					break;
				}
				else
				{
					std::cout << " YTaskGraphError " << std::endl;
					break;
				}
			}
		}

	};
	
	TestFunc(100000);
	TestFunc(200000);
	TestFunc(300000);
	TestFunc(400000);
	TestFunc(500000);


}

