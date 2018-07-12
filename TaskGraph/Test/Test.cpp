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


int main()
{
	FPlatformTime::InitTiming();

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
					std::cout << pMainGFX->Name << std::endl;
					if (pMainGFX->ParentGFX)
					{
						pMainGFX->Pos = pMainGFX->ParentGFX->Pos + 1;
					}

					for (YGFX* pChild : pMainGFX->Children)
					{
						YGFXJob* pJob = ITask::CreateJob<YGFXJob>(nullptr, pChild);
						pJob->DispatchJob();
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
		FPlatformProcess::Sleep(3);
	}

	double fEndTime = FPlatformTime::Seconds();
	std::cout << "GFX Tick Cost Time : " << fEndTime - fStart << std::endl;
}