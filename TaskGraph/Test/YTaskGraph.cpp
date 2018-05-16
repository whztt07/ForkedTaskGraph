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
			YJob* JobToDo = JobArray.Pop();
			if (JobToDo)
			{
				IsIdleState = false;
				FRunnableThread * Thread = FRunnableThread::GetRunnableThread();
				int ThreadID = Thread->GetThreadID();
				JobToDo->ExcuteTask(ThreadID);
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
	FCriticalSection			CS;
};



YJob::YJob()
{
	PrerequistsCounter.Set(1);
}





YJobHandleRef YJob::DispatchJob()
{
	YJobHandleRef JobRefBeforeRelease = JobHandle;
	YTaskGraphInterface::Get().DispatchJob(this);
	return JobRefBeforeRelease;
}

void YJob::ExcuteTask(int InThreadI)
{
	assert(!JobHandle->WaitForJobs.size());
	Task(InThreadI,JobHandle);
	JobHandle->DispatchSubsequents();
	delete this;
}

YJobHandleRef YJob::GetJobHandle()
{
	return JobHandle;
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

void YJobHandle::DoNotCompleteUnitl(YJobHandleRef JobHandleToWaitFor)
{
	WaitForJobs.push_back(JobHandleToWaitFor);
}
bool YJobHandle::AddChildJob(YJob* Child)
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
void YJobHandle::DispatchSubsequents()
{
	if (WaitForJobs.size())
	{
		std::vector<YJobHandleRef> Tmp;
		Tmp.swap(WaitForJobs);
		YJob::CreateJob<YJobNullTask>(YJobHandleRef(this), &Tmp)->DispatchJob();
		return;
	}
	std::vector<YJob*> ChildrenJobs = SubsequenceJobs.GetArrayValueAndClosed();
	for (YJob* ChildJob : ChildrenJobs)
	{
		assert(ChildJob);
		YTaskGraphInterface::Get().DispatchJob(ChildJob);
	}
}

bool YJobHandle::IsComplelte()
{
	return SubsequenceJobs.IsClosed();
}

YJobHandle::YJobHandle()
{

}

YJobHandle::~YJobHandle()
{

}

YJobNullTask::YJobNullTask()
{

}

void YJobNullTask::Task(int InThreadID,YJobHandleRef)
{

}


class DependentJob :public YJob
{
public:
	DependentJob(DependentJob* InParentJob)
		:ParentJob(InParentJob)
	{
		nData = 0;
	}
	virtual void Task(int InThreadID, YJobHandleRef ThisJobHandle) override
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



class YGFX 
{
public:
	YGFX()
	{
		ParentGFX = nullptr;
		Pos = 0;
	}
	std::string Name;
	YGFX* ParentGFX;
	std::vector<YGFX*> Children;
	int  Pos;
	YJobHandleRef  GFXWaitForTickComplelte;

	void Tick()
	{
		class YGFXJob : public YJob
		{
		public :
			YGFXJob(YGFX* pGFX)
			{
				pMainGFX = pGFX;
			}
			virtual  void Task(int InThreadID, YJobHandleRef ThisJobHandle) override
			{
				assert(pMainGFX);
				std::cout << "gfx[" << pMainGFX->Name << "] is tick" << std::endl;
				if (pMainGFX->ParentGFX)
				{
					pMainGFX->Pos = pMainGFX->ParentGFX->Pos + 1;
				}

			/*	FGraphEventRef WaitCallBack = TGraphTask<GFXTickCallBack>::CreateTask(NULL, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(pMainGFX);
				MyCompletionGraphEvent->DontCompleteUntil(WaitCallBack);*/
				for (YGFX* pChild : pMainGFX->Children)
				{
					//TGraphTask<GFXTickTask>::CreateTask(nullptr, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(pChild);
					YGFXJob* pJob = YJob::CreateJob<YGFXJob>(nullptr, pChild);
					YJobHandleRef ChildJobHandle = pJob->DispatchJob();
					ThisJobHandle->DoNotCompleteUnitl(ChildJobHandle);
				}
			}
			YGFX* pMainGFX;
		};

		GFXWaitForTickComplelte = YJob::CreateJob<YGFXJob>(nullptr, this)->DispatchJob();
	}
	
};

void YTaskGraphTest()
{
	YTaskGraphInterface::Startup(4);
	DependentJob* Job0 = YJob::CreateJob<DependentJob>(nullptr, nullptr);
	std::vector<YJobHandleRef> Parent;
	Parent.clear();
	Parent.push_back(Job0->GetJobHandle());
	DependentJob* Job1 = YJob::CreateJob<DependentJob>(&Parent, Job0);
	Parent.clear();
	Parent.push_back(Job1->GetJobHandle());
	DependentJob* Job2 = YJob::CreateJob<DependentJob>(&Parent, Job1);
	YTaskGraphInterface::Get().DispatchJob(Job2);
	YTaskGraphInterface::Get().DispatchJob(Job1);
	YTaskGraphInterface::Get().DispatchJob(Job0);
	Parent.clear();
	Parent.push_back(Job2->GetJobHandle());
	{
		FScopedEvent Event;
		TrigerEventJob* JobEvent = YJob::CreateJob<TrigerEventJob>(&Parent, Event.Get());
		YTaskGraphInterface::Get().DispatchJob(JobEvent);
	}

	std::cout << "Begin do dependance job" << std::endl;
	//                     Root
	//       FirstChild               SecondChild
	//          A                      B                   C
	//      D      E               F      G
	//   H    I      J           K  L     M     
	YGFX Root;
	Root.Name = "ROOT";
	Root.ParentGFX = nullptr;
	Root.Pos = 0;

	YGFX FirstChild;
	FirstChild.Name = "FirstChild";
	FirstChild.ParentGFX = &Root;
	Root.Children.push_back(&FirstChild);


	YGFX SecondChild;
	SecondChild.Name = "SecondChild";
	SecondChild.ParentGFX = &Root;
	Root.Children.push_back(&SecondChild);

	YGFX A;
	A.Name = "A";
	A.ParentGFX = &FirstChild;
	FirstChild.Children.push_back(&A);

	YGFX B;
	B.Name = "B";
	B.ParentGFX = &SecondChild;
	SecondChild.Children.push_back(&B);

	YGFX C;
	C.Name = "C";
	C.ParentGFX = &SecondChild;
	SecondChild.Children.push_back(&C);

	YGFX D;
	D.Name = "D";
	D.ParentGFX = &A;
	A.Children.push_back(&D);

	YGFX E;
	E.Name = "E";
	E.ParentGFX = &A;
	A.Children.push_back(&E);

	YGFX F;
	F.Name = "F";
	F.ParentGFX = &B;
	B.Children.push_back(&F);

	YGFX G;
	G.Name = "G";
	G.ParentGFX = &B;
	B.Children.push_back(&G);

	YGFX H;
	H.Name = "H";
	H.ParentGFX = &D;
	D.Children.push_back(&H);

	YGFX I;
	I.Name = "I";
	I.ParentGFX = &D;
	D.Children.push_back(&I);


	//                     Root
	//       FirstChild               SecondChild
	//          A                      B                   C
	//      D      E               F      G
	//   H    I      J           K  L     M     
	//   

	YGFX J;
	J.Name = "J";
	J.ParentGFX = &E;
	E.Children.push_back(&J);

	YGFX K;
	K.Name = "K";
	K.ParentGFX = &F;
	F.Children.push_back(&K);

	YGFX L;
	L.Name = "L";
	L.ParentGFX = &F;
	F.Children.push_back(&L);

	YGFX M;
	M.Name = "M";
	M.ParentGFX = &G;
	G.Children.push_back(&M);


	Root.Tick();

	{
		FScopedEvent Event;
		Parent.clear();
		Parent.push_back(Root.GFXWaitForTickComplelte);
		TrigerEventJob* JobEvent = YJob::CreateJob<TrigerEventJob>(&Parent, Event.Get());
		YTaskGraphInterface::Get().DispatchJob(JobEvent);
	}

}

void TrigerEventJob::Task(int InThreadIDY, YJobHandleRef ThisJobHandle)
{
	if (Event)
	{
		Event->Trigger();
	}
}