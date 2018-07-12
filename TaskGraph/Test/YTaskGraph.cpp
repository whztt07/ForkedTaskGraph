#include "YTaskGraph.h"
#include <iostream>
#include <assert.h>
#include "Runnable.h"
#include "RunnableThread.h"
#include "Event.h"
#include <cstdlib>
#include "ThreadSafeCounter.h"
#include "ScopedEvent.h"
float GetRundomTime()
{
	return (float)(std::rand()) / 1000000.0f;
}

class YTaskGraphThread : public FRunnableThread
{
public:
	friend class YTaskGraphThreadProc;
	virtual void SetThreadPriority(EThreadPriority NewPriority) override{}
	virtual void Suspend(bool bShouldPause) override{}
	virtual bool Kill(bool bShouldWait = true) override{}
	virtual void WaitForCompletion() override{}
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
			ITask* JobToDo = nullptr;
			{
				FScopeLock Lock(&CS);
				JobToDo = 	JobArray.Pop();
			}
			if (JobToDo)
			{
				IsIdleState = false;
				FRunnableThread * Thread = FRunnableThread::GetRunnableThread();
				int ThreadID = Thread->GetThreadID();
				JobToDo->ExcuteTask(ThreadID);
			}
			else
			{
				IsIdleState = true;
				EventWaitforIdle->Wait(20000);
			}
		}
	}
	void AddJob(ITask* InJob)
	{
		
			FScopeLock Lock(&CS);
			size_t JobsToDoNum = JobArray.Num();
			assert(InJob->GetJobHandle() != nullptr);
			JobArray.Push(InJob);
			if (JobsToDoNum == 0)
			{
				EventWaitforIdle->Trigger();
			}
		
	}
	bool IsIdle() const { return IsIdleState; }
	ThreadSafeLockPointerArray<ITask> JobArray;
	FEvent*					EventWaitforIdle;
	bool							IsIdleState;
	FCriticalSection			CS;
};

YJobHandleRef ITask::DispatchJob()
{
	YJobHandleRef JobRefBeforeRelease = Handle;
	YTaskGraphInterface::Get().DispatchJob(this);
	return JobRefBeforeRelease;
}

void ITask::ExcuteTask(int InThreadI)
{
	assert(!Handle->WaitForJobs.size());
	Task(InThreadI,Handle);
	Handle->DispatchSubsequents();
	delete this;
}

YJobHandleRef ITask::GetJobHandle()
{
	return Handle;
}

class YTaskGraphInterfaceImplement : public YTaskGraphInterface
{
public:
	YTaskGraphInterfaceImplement();
	virtual	void DispatchJob(ITask * JobToDispatch) override;
private:
	void EnqueueJob(ITask* JobToDispatch);
	std::vector<YTaskGraphThread*> Threads;
	std::vector<YTaskGraphThreadProc*>   ThreadProcs;
	FThreadSafeCounter         ThreadToDispatchImp;
};

void YTaskGraphInterfaceImplement::DispatchJob(ITask * JobToDispatch)
{
	if (JobToDispatch->ParentsNum.Decrement() == 0)
	{
		EnqueueJob(JobToDispatch);
	}
}

void YTaskGraphInterfaceImplement::EnqueueJob(ITask * JobToDispatch)
{
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
	const int nThread = 8;
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
	return *GYTaskGraphInterfaceImplement;
}

void TaskHandle::DoNotCompleteUnitl(YJobHandleRef JobHandleToWaitFor)
{
	WaitForJobs.push_back(JobHandleToWaitFor);
}

bool TaskHandle::AddChildJob(ITask* Child)
{
	if (Childrens.IsClosed())
	{
		return false;
	}
	else
	{
		Childrens.Push(Child);
		return true;
	}
}

void TaskHandle::DispatchSubsequents()
{
	if (WaitForJobs.size())
	{
		std::vector<YJobHandleRef> Tmp;
		Tmp.swap(WaitForJobs);
		ITask::CreateTask<YJobNullTask>(YJobHandleRef(this), &Tmp)->DispatchJob();
		return;
	}
	std::vector<ITask*> ChildrenJobs = Childrens.GetArrayValueAndClosed();
	for (ITask* ChildJob : ChildrenJobs)
	{
		YTaskGraphInterface::Get().DispatchJob(ChildJob);
	}
}

bool TaskHandle::IsComplelte()
{
	return Childrens.IsClosed();
}

void TrigerEventJob::Task(int InThreadIDY, const YJobHandleRef& ThisJobHandle)
{
	if (Event)
	{
		Event->Trigger();
	}
}