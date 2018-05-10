#include "YTaskGraph.h"
#include <iostream>
#include <assert.h>
#include "Runnable.h"
#include "RunnableThread.h"
#include "Event.h"
#include <algorithm>
#include <cstdlib>
#include "ThreadSafeCounter.h"
#include <string>
#include "ScopedEvent.h"
float GetRundomTime()
{
	return (float)(std::rand()) / 1000000.0f;
}
ThreadSafeLockValueArray<int> GComputeResultTest;
FThreadSafeCounter GTaskCounter;
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
		//GComputeResultTest.Push(JobID);
		GTaskCounter.Increment();
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
		IsIdleState = true;
	}

	virtual unsigned int Run() override
	{
		while (1)
		{
			YJob* JobToDo = JobArray.Pop();
			if (JobToDo)
			{
				IsIdleState = false;
				FRunnableThread * Thread = FRunnableThread::GetRunnableThread();
				int ThreadID = Thread->GetThreadID();
				JobToDo->Task(ThreadID);
				JobToDo->EndJob();
				//JobToDo->~YJob();
				//delete JobToDo;
			}
			else
			{
				IsIdleState = true;
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
	bool IsIdle() const { return IsIdleState; }
	ThreadSafeLockPointerArray<YJob> JobArray;
	FEvent*					EventWaitforIdle;
	bool							IsIdleState;
};



YJob::YJob()
{
	PrerequistsCounter.Set(1);
}

bool YJob::AddChildJob(YJob* Child)
{
	if (SubsequenceJobs.IsClosed())
	{
		return false;
	}
	else
	{
		SubsequenceJobs.Push(Child);
		return true;
	}
}

void YJob::EndJob()
{
	std::vector<YJob*> ChildrenJobs = SubsequenceJobs.GetArrayValueAndClosed();
	for (YJob* ChildJob : ChildrenJobs)
	{
		assert(ChildJob);
		YTaskGraphInterface::Get().DispatchJob(ChildJob);
	}
}



class YTaskGraphInterfaceImplement : public YTaskGraphInterface
{
public:
	YTaskGraphInterfaceImplement();
	virtual	void DispatchJob(YJob * JobToDispatch) override;
private:
	void EnqueueJob(YJob* JobToDispatch);
	std::vector<YTaskGraphThread*> Threads;
	std::vector<YTaskGraphThreadProc*>   ThreadProcs;
	FThreadSafeCounter         ThreadToDispatchImp;
};


void YTaskGraphInterfaceImplement::DispatchJob(YJob * JobToDispatch)
{
	assert(JobToDispatch);
	if (JobToDispatch->PrerequistsCounter.Decrement() == 0)
	{
		EnqueueJob(JobToDispatch);
	}
}

void YTaskGraphInterfaceImplement::EnqueueJob(YJob * JobToDispatch)
{
	assert(JobToDispatch);
	// find an idle thread;
	YTaskGraphThreadProc* ThreadRunnableToDispatch = nullptr;
	for (YTaskGraphThreadProc* ThreadRunnable : ThreadProcs)
	{
		if (ThreadRunnable->IsIdle())
		{
			ThreadRunnableToDispatch = ThreadRunnable;
			break;
		}
	}
	if (ThreadRunnableToDispatch == nullptr)
	{
		int IndexOfThreadProc = ThreadToDispatchImp.Increment() % ThreadProcs.size();
		ThreadRunnableToDispatch = ThreadProcs[IndexOfThreadProc];
	}
	assert(ThreadRunnableToDispatch);

	ThreadRunnableToDispatch->AddJob(JobToDispatch);
}

void YTaskGraphInterface::Startup(int NumThreads)
{
	new YTaskGraphInterfaceImplement();
}

static YTaskGraphInterface* GYTaskGraphInterfaceImplement;

YTaskGraphInterfaceImplement::YTaskGraphInterfaceImplement()
{
	const int nThread = 4;
	GYTaskGraphInterfaceImplement = this;
	for (int i = 0; i < nThread; ++i)
	{
		ThreadProcs.push_back(new YTaskGraphThreadProc());
		Threads.push_back((YTaskGraphThread*)YTaskGraphThread::Create(ThreadProcs[i], TEXT("AnyThread")));
	}
	ThreadToDispatchImp.Reset();
}

YTaskGraphInterface& YTaskGraphInterface::Get()
{
	assert(GYTaskGraphInterfaceImplement);
	return *GYTaskGraphInterfaceImplement;
}

class DependentJob :public YJob
{
public:
	DependentJob(DependentJob* InParentJob)
		:ParentJob(InParentJob)
	{
		nData = 0;
	}
	virtual void Task(int InThreadID) override
	{
		if (ParentJob)
		{
			nData = ParentJob->nData+1;
		}
		std::cout << "[DepenetnJob] " << nData << "[Thread ID]"<< InThreadID << std::endl;
	}
private:
	DependentJob* ParentJob;
	int nData;
};

class TrigerEventJob :public YJob
{
public:
	TrigerEventJob(FEvent* pEvent)
	{
		Event = pEvent;
	}
	virtual void Task(int InThreadID) override
	{
		if (Event)
		{
			Event->Trigger();
		}
	}
	FEvent* Event;
};
void YTaskGraphTest()
{
	YTaskGraphInterface::Startup(4);
	DependentJob* Job0 = YTaskGraphInterface::CreateTask<DependentJob>(nullptr, nullptr);
	std::vector<YJob*> Parent;
	Parent.clear();
	Parent.push_back(Job0);
	DependentJob* Job1 = YTaskGraphInterface::CreateTask<DependentJob>(&Parent, Job0);
	Parent.clear();
	Parent.push_back(Job1);
	DependentJob* Job2 = YTaskGraphInterface::CreateTask<DependentJob>(&Parent, Job1);
	YTaskGraphInterface::Get().DispatchJob(Job2);
	YTaskGraphInterface::Get().DispatchJob(Job1);
	YTaskGraphInterface::Get().DispatchJob(Job0);
	Parent.clear();
	Parent.push_back(Job2);
	{
		FScopedEvent Event;
		TrigerEventJob* JobEvent = YTaskGraphInterface::CreateTask<TrigerEventJob>(&Parent, Event.Get());
		YTaskGraphInterface::Get().DispatchJob(JobEvent);
	}
}
