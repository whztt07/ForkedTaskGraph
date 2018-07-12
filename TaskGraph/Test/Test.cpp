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
	
	//DependentJob* Job0 = YJob::CreateJob<DependentJob>(nullptr, nullptr);
	//std::vector<YJobHandleRef> Parent;
	//Parent.clear();
	//Parent.push_back(Job0->GetJobHandle());
	//DependentJob* Job1 = YJob::CreateJob<DependentJob>(&Parent, Job0);
	//Parent.clear();
	//Parent.push_back(Job1->GetJobHandle());
	//DependentJob* Job2 = YJob::CreateJob<DependentJob>(&Parent, Job1);
	//YTaskGraphInterface::Get().DispatchJob(Job2);
	//YTaskGraphInterface::Get().DispatchJob(Job1);
	//YTaskGraphInterface::Get().DispatchJob(Job0);
	//Parent.clear();
	//Parent.push_back(Job2->GetJobHandle());
	//{
	//	FScopedEvent Event;
	//	TrigerEventJob* JobEvent = YJob::CreateJob<TrigerEventJob>(&Parent, Event.Get());
	//	YTaskGraphInterface::Get().DispatchJob(JobEvent);
	//}

	

	

}

int main()
{
	//RedirectionIOToFile();
	FPlatformTime::InitTiming();
	/*const int nStep = 10;
	const int nTask = 3000;
	for (int i = 0; i < nTask; ++i)
	{
		GPrimeResult.push_back(std::vector<int>());
	}
	double fStart = FPlatformTime::Seconds();
	std::vector<FGraphEventRef> Result;
	for (int i = 0; i < nTask; ++i)
	{
		int nStart = i*nStep;
		int nEnd = (i + 1)*nStep;
		Result.push_back(TGraphTask<FindPrim>::CreateTask(NULL, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(nStart, nEnd, i));
	}

	{
		FScopedEvent WaitForTasks;
		TGraphTask<FTriggerEventGraphTask>::CreateTask(&Result, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(WaitForTasks.Get());
	}
	
	double fEndTime = FPlatformTime::Seconds();
	std::cout<<"cost time "<<fEndTime - fStart<<std::endl;*/
	/*int BreakStep = 0;
	for (int i = 0; i < nTask; ++i)
	{
		for (int j = 0; j < GPrimeResult[i].size(); ++j)
		{
			BreakStep++;
			if (BreakStep % 20 == 0)
				std::cout << std::endl;
			std::cout << GPrimeResult[i][j] << "  ";
		}
	}*/


	/*std::cout<<"Begin do dependance job"<<std::endl;*/
	//                     Root
	//       FirstChild               SecondChild
	//          A                      B                   C
	//      D      E               F      G
	//   H    I      J           K  L     M     
	//GFX Root;
	//Root.Name = "ROOT";
	//Root.ParentGFX = nullptr;
	//Root.Pos = 0;

	//GFX FirstChild;
	//FirstChild.Name = "FirstChild";
	//FirstChild.ParentGFX = &Root;
	//Root.Children.push_back(&FirstChild);


	//GFX SecondChild;
	//SecondChild.Name = "SecondChild";
	//SecondChild.ParentGFX = &Root;
	//Root.Children.push_back(&SecondChild);

	//GFX A;
	//A.Name = "A";
	//A.ParentGFX = &FirstChild;
	//FirstChild.Children.push_back(&A);

	//GFX B;
	//B.Name = "B";
	//B.ParentGFX = &SecondChild;
	//SecondChild.Children.push_back(&B);

	//GFX C;
	//C.Name = "C";
	//C.ParentGFX = &SecondChild;
	//SecondChild.Children.push_back(&C);

	//GFX D;
	//D.Name = "D";
	//D.ParentGFX = &A;
	//A.Children.push_back(&D);

	//GFX E;
	//E.Name = "E";
	//E.ParentGFX = &A;
	//A.Children.push_back(&E);

	//GFX F;
	//F.Name = "F";
	//F.ParentGFX = &B;
	//B.Children.push_back(&F);

	//GFX G;
	//G.Name = "G";
	//G.ParentGFX = &B;
	//B.Children.push_back(&G);

	//GFX H;
	//H.Name = "H";
	//H.ParentGFX = &D;
	//D.Children.push_back(&H);

	//GFX I;
	//I.Name = "I";
	//I.ParentGFX = &D;
	//D.Children.push_back(&I);


	////                     Root
	////       FirstChild               SecondChild
	////          A                      B                   C
	////      D      E               F      G
	////   H    I      J           K  L     M     
	////   

	//GFX J;
	//J.Name = "J";
	//J.ParentGFX = &E;
	//E.Children.push_back(&J);

	//GFX K;
	//K.Name = "K";
	//K.ParentGFX = &F;
	//F.Children.push_back(&K);

	//GFX L;
	//L.Name = "L";
	//L.ParentGFX = &F;
	//F.Children.push_back(&L);

	//GFX M;
	//M.Name = "M";
	//M.ParentGFX = &G;
	//G.Children.push_back(&M);


	//Root.Tick();
	//
	//std::vector<FGraphEventRef> WaitList;
	//WaitList.push_back(Root.GFXWaitForTickComplete);
	//TGraphTask<FReturnGraphTask>::CreateTask(&WaitList, ENamedThreads::GameThread).ConstructAndDispatchWhenReady(ENamedThreads::GameThread);
	//FTaskGraphInterface::Get().ProcessThreadUntilRequestReturn(ENamedThreads::GameThread);

	//std::cout<<"End do dependance job\n"<<std::endl;
	//FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);

	//AllocResource();
	//fStart = FPlatformTime::Seconds();
	//Merge();
	//fEndTime = FPlatformTime::Seconds();
	//std::cout << "merge sort one core cost time " << fEndTime - fStart << std::endl;

	//double fStart = FPlatformTime::Seconds();
	//MergeParallel();
	//double fEndTime = FPlatformTime::Seconds();
	//std::cout << "merge sort taskgraph cost time " << fEndTime - fStart << std::endl;
	//CompareResult();

	//fStart = FPlatformTime::Seconds();
	//QSort();
	//fEndTime = FPlatformTime::Seconds();
	//std::cout << "qsort cost time " << fEndTime - fStart << std::endl;


	//YTaskGraphTest();
	

	//fStart = FPlatformTime::Seconds();
	//MergeParallelWithY();
	//fEndTime = FPlatformTime::Seconds();
	//std::cout << "merge sort YJob cost time " << fEndTime - fStart << std::endl;
	//CompareResultY();
	//EndRedirectionIoToFile();
	//std::cout << "EndProcess..." << std::endl;

	YTaskGraphInterface::Startup(4);

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
			class YGFXJob : public ITask
			{
			public:
				YGFXJob(YGFX* pGFX)
				{
					pMainGFX = pGFX;
				}
				virtual  void Task(int InThreadID, const YJobHandleRef& ThisJobHandle) override
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
						YGFXJob* pJob = ITask::CreateJob<YGFXJob>(nullptr, pChild);
						YJobHandleRef ChildJobHandle = pJob->DispatchJob();
						ThisJobHandle->DoNotCompleteUnitl(ChildJobHandle);
					}
				}
				YGFX* pMainGFX;
			};

			GFXWaitForTickComplelte = ITask::CreateJob<YGFXJob>(nullptr, this)->DispatchJob();
		}

	};

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

	double fStart = FPlatformTime::Seconds();

	Root.Tick();

	{
		FEvent* pEvent = FEventPool<EEventPoolTypes::AutoReset>::Get().GetEventFromPool();
		std::vector<YJobHandleRef>   Parent;
		Parent.push_back(Root.GFXWaitForTickComplelte);
		TrigerEventJob* JobEvent = ITask::CreateJob<TrigerEventJob>(&Parent, pEvent);
		YTaskGraphInterface::Get().DispatchJob(JobEvent);
		pEvent->Wait();
		FEventPool<EEventPoolTypes::AutoReset>::Get().ReturnToPool(pEvent);
	}

	double fEndTime = FPlatformTime::Seconds();
	std::cout << "GFX Tick Cost Time : " << fEndTime - fStart << std::endl;
}