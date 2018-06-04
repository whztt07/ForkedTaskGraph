#include "TaskGraphInterfaces.h"
#include "YTaskGraph.h"
#include "AsycComputeTree.h"
#include "ScopedEvent.h"
#include <iostream>
#include "GenericPlatformTime.h"


struct FFebTaskAdd
{
	unsigned __int64*  Feb;
	unsigned __int64*  Nth1;
	unsigned __int64*  Nth2;
	FFebTaskAdd(unsigned __int64*  InFeb, unsigned __int64* InNth1,unsigned __int64* InNth2)
		:Feb(InFeb)
		, Nth1(InNth1)
		,Nth2(InNth2)
	{

	}


	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}

	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::TrackSubsequents;
	}
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		assert(Feb);
		assert(Nth1);
		assert(Nth2);
		*Feb = *Nth1 + *Nth2;
		delete Nth1;
		delete Nth2;
	}

};
struct FFebTask
{
		unsigned __int64*  Feb;
		unsigned __int64  Nth;
		FFebTask(unsigned __int64  InNth, unsigned __int64* OutFeb)
			:Nth(InNth)
			, Feb(OutFeb)
		{

		}


		static ENamedThreads::Type GetDesiredThread()
		{
			return ENamedThreads::AnyThread;
		}

		static ESubsequentsMode::Type GetSubsequentsMode()
		{
			return ESubsequentsMode::TrackSubsequents;
		}
		void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
		{
			assert(Feb);
			if (Nth < 2)
			{
				*Feb = Nth;
			}
			else
			{
				unsigned __int64* N1Result =  new unsigned __int64(0);
				unsigned __int64* N2Result = new unsigned __int64(0);
				FGraphEventRef LeftN1 = TGraphTask<FFebTask>::CreateTask(nullptr, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(Nth-1,N1Result);
				FGraphEventRef LeftN2 = TGraphTask<FFebTask>::CreateTask(nullptr, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(Nth-2,N2Result);
				std::vector<FGraphEventRef> Depenece = { LeftN1,LeftN2 };
				MyCompletionGraphEvent->DontCompleteUntil(TGraphTask<FFebTaskAdd>::CreateTask(&Depenece, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(Feb, N1Result, N2Result));
			}
		}
};
unsigned __int64 CaculateFebFTaskGraph(unsigned __int64 n)
{
	unsigned __int64 Feb = 0;
	{
		FScopedEvent Event;
		FGraphEventRef Result = TGraphTask<FFebTask>::CreateTask(nullptr, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(n, &Feb);
		std::vector<FGraphEventRef> Depeneces = { Result };
		TGraphTask<FTriggerEventGraphTask>::CreateTask(&Depeneces, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(Event.Get());
	}
	return Feb;
}

struct YFebTaskAdd :public YJob
{
	unsigned __int64*  Feb;
	unsigned __int64*  Nth1;
	unsigned __int64*  Nth2;
	YFebTaskAdd(unsigned __int64*  InFeb, unsigned __int64* InNth1, unsigned __int64* InNth2)
		:Feb(InFeb)
		, Nth1(InNth1)
		, Nth2(InNth2)
	{

	}

	virtual void Task(int InThreadID, const YJobHandleRef& ThisJobHandle)
	{
		assert(Feb);
		assert(Nth1);
		assert(Nth2);
		*Feb = *Nth1 + *Nth2;
		delete Nth1;
		delete Nth2;
	}
};


struct YFebTask :public YJob
{
	unsigned __int64*  Feb;
	unsigned __int64  Nth;
	YFebTask(unsigned __int64  InNth, unsigned __int64* OutFeb)
		:Nth(InNth)
		, Feb(OutFeb)
	{

	}


	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}

	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::TrackSubsequents;
	}
	virtual void Task(int InThreadID, const YJobHandleRef& ThisJobHandle)
	{
		assert(Feb);
		if (Nth < 2)
		{
			*Feb = Nth;
		}
		else
		{
			unsigned __int64* N1Result = new unsigned __int64(0);
			unsigned __int64* N2Result = new unsigned __int64(0);
			//FGraphEventRef LeftN1 = TGraphTask<FFebTask>::CreateTask(nullptr, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(Nth - 1, N1Result);
			//FGraphEventRef LeftN2 = TGraphTask<FFebTask>::CreateTask(nullptr, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(Nth - 2, N2Result);
			//std::vector<FGraphEventRef> Depenece = { LeftN1,LeftN2 };
			//MyCompletionGraphEvent->DontCompleteUntil(TGraphTask<FFebTaskAdd>::CreateTask(&Depenece, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(Feb, N1Result, N2Result));

			YJobHandleRef LeftN1 = YJob::CreateJob<YFebTask>(nullptr, Nth - 1, N1Result)->DispatchJob();
			YJobHandleRef LeftN2 = YJob::CreateJob<YFebTask>(nullptr, Nth - 2, N2Result)->DispatchJob();
			std::vector<YJobHandleRef> Dependence = { LeftN1,LeftN2 };
			ThisJobHandle->DoNotCompleteUnitl(YJob::CreateJob<YFebTaskAdd>(&Dependence, Feb, N1Result, N2Result)->DispatchJob());
		}
	}
};
unsigned __int64 CaculateFebYTaskGraph(unsigned __int64 n)
{
	unsigned __int64 Feb = 0;
	{
		FScopedEvent Event;
		YJobHandleRef Result = YJob::CreateJob<YFebTask>(nullptr, n, &Feb)->DispatchJob();
		std::vector<YJobHandleRef> Depeneces = { Result };
		YJob::CreateJob<TrigerEventJob>(&Depeneces,Event.Get())->DispatchJob();
	}
	return Feb;
}

void TestFeb()
{
	double fStart = FPlatformTime::Seconds();
	unsigned __int64 Nth = 30;
	unsigned __int64 nResult = CaculateFebFTaskGraph(Nth);
	double fEndTime = FPlatformTime::Seconds();
	std::cout << "FFeb[ " << Nth << "] :" << nResult << "   cost "<< fEndTime-fStart<< std::endl;
	
	fStart = FPlatformTime::Seconds();
	nResult = CaculateFebYTaskGraph(Nth);
	fEndTime = FPlatformTime::Seconds();
	std::cout << "YFeb[ " << Nth << "] :" << nResult << "   cost "<< fEndTime-fStart<< std::endl;

}
