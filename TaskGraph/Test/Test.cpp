#include "TaskGraphInterfaces.h"
#include "WindowsPlatformProcess.h"
#include "ScopedEvent.h"
#include "GenericPlatformTime.h"
#include <iostream>
#include <cstdio>
#include <string>
#include "GFX.h"
#include "AsycComputeTree.h"
#include "Runnable.h"
#include "RunnableThread.h"
#include "ScopeLock.h"
#include <vector>
#include "YTaskGraph.h"
#include <fstream>
#include "EventPool.h"

bool IsPrim(int n)
{
	if (n == 2)
		return true;
	for (int i = 2; i <= (int)sqrt(n); ++i)
	{
		if (n%i == 0)
		{
			return false;
		}
	}
	return true;
}

std::vector<std::vector<int>> GPrimeResult;
struct FindPrim
{
	FindPrim(int nStart, int nEnd, int iResult)
	{
		Start = nStart;
		End = nEnd;
		ResultIndex = iResult;
	}
	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}
	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		// The arguments are useful for setting up other tasks.
		// Do work here, probably using SomeArgument.
		//MyCompletionGraphEvent->DontCompleteUntil(TGraphTask<FSomeChildTask>::CreateTask(NULL, CurrentThread).ConstructAndDispatchWhenReady());
		GPrimeResult[ResultIndex].reserve(End - Start);
		for (int i = Start; i < End; ++i)
		{
			if (IsPrim(i))
			{
				GPrimeResult[ResultIndex].push_back(i);
			}
		}
	}
	int Start;
	int End;
	int ResultIndex;
};

std::ofstream of("out.txt");
std::streambuf* coutBuf;
void RedirectionIOToFile()
{
	coutBuf = std::cout.rdbuf();
	// 获取文件out.txt流缓冲区指针  
	std::streambuf* fileBuf = of.rdbuf();

// 设置cout流缓冲区指针为out.txt的流缓冲区指针  
	std::cout.rdbuf(fileBuf);
}

void EndRedirectionIoToFile()
{
	of.flush();
	of.close();

	// 恢复cout原来的流缓冲区指针  
	std::cout.rdbuf(coutBuf);
}

class DependentJob :public ITask
{
public:
	DependentJob(DependentJob* InParentJob)
		:ParentJob(InParentJob)
	{
		nData = 0;
	}
	virtual void Task(int InThreadID, const YJobHandleRef& ThisJobHandle) override
	{
		if (ParentJob)
		{
			nData = ParentJob->nData + 1;
		}
		std::cout << "[DepenetnJob] " << nData << "[Thread ID]" << InThreadID << std::endl;
	}
private:
	DependentJob* ParentJob;
	int nData;
};




void YTaskGraphTest()
{
	
	//DependentJob* Job0 = YJob::CreateTask<DependentJob>(nullptr, nullptr);
	//std::vector<YJobHandleRef> Parent;
	//Parent.clear();
	//Parent.push_back(Job0->GetJobHandle());
	//DependentJob* Job1 = YJob::CreateTask<DependentJob>(&Parent, Job0);
	//Parent.clear();
	//Parent.push_back(Job1->GetJobHandle());
	//DependentJob* Job2 = YJob::CreateTask<DependentJob>(&Parent, Job1);
	//YTaskGraphInterface::Get().DispatchJob(Job2);
	//YTaskGraphInterface::Get().DispatchJob(Job1);
	//YTaskGraphInterface::Get().DispatchJob(Job0);
	//Parent.clear();
	//Parent.push_back(Job2->GetJobHandle());
	//{
	//	FScopedEvent Event;
	//	TrigerEventJob* JobEvent = YJob::CreateTask<TrigerEventJob>(&Parent, Event.Get());
	//	YTaskGraphInterface::Get().DispatchJob(JobEvent);
	//}

	

	

}

int main()
{
	//RedirectionIOToFile();
	FPlatformTime::InitTiming();
	YTaskGraphInterface::Startup(4);
	FTaskGraphInterface::Get().Startup(4);
	double fStart = FPlatformTime::Seconds();
	
	double fEndTime = FPlatformTime::Seconds();

	AllocResource();
	fStart = FPlatformTime::Seconds();
	MergeRecursive();
	fEndTime = FPlatformTime::Seconds();
	std::cout << "merge sort one core cost time " << fEndTime - fStart << std::endl;

	fStart = FPlatformTime::Seconds();
	MergeParallelUE();
	fEndTime = FPlatformTime::Seconds();
	std::cout << "merge sort UE taskgraph cost time " << fEndTime - fStart << std::endl;
	CompareResult();

	fStart = FPlatformTime::Seconds();
	QSort();
	fEndTime = FPlatformTime::Seconds();
	std::cout << "qsort cost time " << fEndTime - fStart << std::endl;


	//YTaskGraphTest();
	

	fStart = FPlatformTime::Seconds();
	MergeParallelWithY();
	fEndTime = FPlatformTime::Seconds();
	std::cout << "merge sort YJob cost time " << fEndTime - fStart << std::endl;
	CompareResultY();
	std::cout << "EndProcess..." << std::endl;
}