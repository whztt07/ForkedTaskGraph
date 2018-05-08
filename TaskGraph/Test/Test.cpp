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
	// ��ȡ�ļ�out.txt��������ָ��  
	std::streambuf* fileBuf = of.rdbuf();

// ����cout��������ָ��Ϊout.txt����������ָ��  
	std::cout.rdbuf(fileBuf);
}

void EndRedirectionIoToFile()
{
	of.flush();
	of.close();

	// �ָ�coutԭ������������ָ��  
	std::cout.rdbuf(coutBuf);
}
int main()
{
	RedirectionIOToFile();
	FTaskGraphInterface::Startup(FPlatformProcess::NumberOfCores());
	FTaskGraphInterface::Get().AttachToThread(ENamedThreads::GameThread);
	FPlatformTime::InitTiming();
	const int nStep = 10;
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
	std::cout<<"cost time "<<fEndTime - fStart<<std::endl;
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


	std::cout<<"Begin do dependance job"<<std::endl;
	//                     Root
	//       FirstChild               SecondChild
	//          A                      B                   C
	//      D      E               F      G
	//   H    I      J           K  L     M     
	GFX Root;
	Root.Name = "ROOT";
	Root.ParentGFX = nullptr;
	Root.Pos = 0;

	GFX FirstChild;
	FirstChild.Name = "FirstChild";
	FirstChild.ParentGFX = &Root;
	Root.Children.push_back(&FirstChild);


	GFX SecondChild;
	SecondChild.Name = "SecondChild";
	SecondChild.ParentGFX = &Root;
	Root.Children.push_back(&SecondChild);

	GFX A;
	A.Name = "A";
	A.ParentGFX = &FirstChild;
	FirstChild.Children.push_back(&A);

	GFX B;
	B.Name = "B";
	B.ParentGFX = &SecondChild;
	SecondChild.Children.push_back(&B);

	GFX C;
	C.Name = "C";
	C.ParentGFX = &SecondChild;
	SecondChild.Children.push_back(&C);

	GFX D;
	D.Name = "D";
	D.ParentGFX = &A;
	A.Children.push_back(&D);

	GFX E;
	E.Name = "E";
	E.ParentGFX = &A;
	A.Children.push_back(&E);

	GFX F;
	F.Name = "F";
	F.ParentGFX = &B;
	B.Children.push_back(&F);

	GFX G;
	G.Name = "G";
	G.ParentGFX = &B;
	B.Children.push_back(&G);

	GFX H;
	H.Name = "H";
	H.ParentGFX = &D;
	D.Children.push_back(&H);

	GFX I;
	I.Name = "I";
	I.ParentGFX = &D;
	D.Children.push_back(&I);


	//                     Root
	//       FirstChild               SecondChild
	//          A                      B                   C
	//      D      E               F      G
	//   H    I      J           K  L     M     
	//   

	GFX J;
	J.Name = "J";
	J.ParentGFX = &E;
	E.Children.push_back(&J);

	GFX K;
	K.Name = "K";
	K.ParentGFX = &F;
	F.Children.push_back(&K);

	GFX L;
	L.Name = "L";
	L.ParentGFX = &F;
	F.Children.push_back(&L);

	GFX M;
	M.Name = "M";
	M.ParentGFX = &G;
	G.Children.push_back(&M);


	Root.Tick();
	
	std::vector<FGraphEventRef> WaitList;
	WaitList.push_back(Root.GFXWaitForTickComplete);
	TGraphTask<FReturnGraphTask>::CreateTask(&WaitList, ENamedThreads::GameThread).ConstructAndDispatchWhenReady(ENamedThreads::GameThread);
	FTaskGraphInterface::Get().ProcessThreadUntilRequestReturn(ENamedThreads::GameThread);

	std::cout<<"End do dependance job\n"<<std::endl;
	//FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);

	AllocResource();
	fStart = FPlatformTime::Seconds();
	Merge();
	fEndTime = FPlatformTime::Seconds();
	std::cout << "merge sort one core cost time " << fEndTime - fStart << std::endl;

	fStart = FPlatformTime::Seconds();
	MergeParallel();
	fEndTime = FPlatformTime::Seconds();
	std::cout << "merge sort taskgraph cost time " << fEndTime - fStart << std::endl;

	fStart = FPlatformTime::Seconds();
	QSort();
	fEndTime = FPlatformTime::Seconds();
	std::cout << "qsort cost time " << fEndTime - fStart << std::endl;
	CompareResult();

	YTaskGraphTest();
	
	EndRedirectionIoToFile();
	std::cout << "EndProcess..." << std::endl;
}