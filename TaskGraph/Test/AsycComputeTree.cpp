#include "AsycComputeTree.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include "TaskGraphInterfaces.h"
#include "ScopedEvent.h"
#include "YTaskGraph.h"

AsycComputeTree::AsycComputeTree()
{
}


AsycComputeTree::~AsycComputeTree()
{
}
std::vector<int>   GVector;
std::vector<int>   GVector2;
std::vector<int>   GVector3;
std::vector<int>   GVector4;

void MergeSortRecursive(int nStart,int nEnd)
{
	if (nEnd - nStart < 100)
	{
		std::sort(GVector.begin() + nStart, GVector.begin() + nEnd);
		return;
	}
	int Mid = (nStart + nEnd) / 2;
	MergeSortRecursive(nStart, Mid);
	MergeSortRecursive(Mid,nEnd);
	std::inplace_merge(GVector.begin() + nStart, GVector.begin() + Mid, GVector.begin() + nEnd);
}
int gCount = 100000000;
void AllocResource()
{
	GVector.reserve(gCount);
	for (int i = 0; i < gCount; ++i)
	{
		GVector.push_back(rand() % 65535);
	}

	GVector2.assign(GVector.begin(), GVector.end());
	GVector3.assign(GVector.begin(), GVector.end());
	GVector4.assign(GVector.begin(), GVector.end());
}

void Merge()
{
	MergeSortRecursive(0, gCount);

	/*for (int i : GVector)
	{
		std::cout << i << "   ";
	}*/
}


struct MergeTask
{
	int nStart;
	int nEnd;
	int nMid;
	MergeTask(int Start, int End, int Mid)
		:nStart(Start)
		, nEnd(End)
		, nMid(Mid)
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
		std::inplace_merge(GVector2.begin() + nStart, GVector2.begin() + nMid, GVector2.begin() + nEnd);
	}

};
struct MergeSortTask
{
public:
	MergeSortTask(int Start, int End)
		:nStart(Start)
		, nEnd(End)
	{}
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
		if (nEnd - nStart < 100)
		{
			std::sort(GVector2.begin() + nStart, GVector2.begin() + nEnd);
			return;
		}
		int Mid = (nStart + nEnd) / 2;
		FGraphEventRef LeftHalfSoft = TGraphTask<MergeSortTask>::CreateTask(nullptr, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(nStart, Mid);
		FGraphEventRef RightHalfSoft = TGraphTask<MergeSortTask>::CreateTask(nullptr, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(Mid, nEnd);
		FGraphEventArray MergePrerequisites = { LeftHalfSoft, RightHalfSoft };
		MyCompletionGraphEvent->DontCompleteUntil(TGraphTask<MergeTask>::CreateTask(&MergePrerequisites, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(nStart, nEnd, Mid));
	}

	int nStart;
	int nEnd;
};


void MergeParallel()
{

	{
		FScopedEvent WaitForTasks;
		std::vector<FGraphEventRef> WaitList;
		WaitList.push_back(TGraphTask<MergeSortTask>::CreateTask(nullptr, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(0, gCount));
		TGraphTask<FTriggerEventGraphTask>::CreateTask(&WaitList, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(WaitForTasks.Get());
		// waitfor(Root.GFXWaitForTickComplete);
	}
	/*for (int i : GVector2)
	{
	std::cout << i << "   ";
	}*/
}

class YMergeJob : public YJob
{
public:
	int nStart;
	int nEnd;
	int nMid;
	YMergeJob(int Start, int End, int Mid)
		:nStart(Start)
		, nEnd(End)
		, nMid(Mid)
	{

	}
private:
	YMergeJob(const YMergeJob&) = delete;
	YMergeJob& operator=(const YMergeJob&) = delete;
	virtual void Task(int InThreadID, const YJobHandleRef& ThisJobHandle)
	{
		std::inplace_merge(GVector4.begin() + nStart, GVector4.begin() + nMid, GVector4.begin() + nEnd);
	}
};

class MergeSortJob: public YJob
{
public:
	MergeSortJob(int Start, int End)
		:nStart(Start)
		, nEnd(End)
	{
		//std::cout << "StartJob:[" << nStart<< "   "<<nEnd<<"]"<<std::endl;
	}
private:
	MergeSortJob(const MergeSortJob&) = delete;
	MergeSortJob& operator=(const MergeSortJob&) = delete;

	virtual void Task(int InThreadID, const YJobHandleRef& ThisJobHandle)
	{
		if (nEnd - nStart < 100)
		{
			std::sort(GVector4.begin() + nStart, GVector4.begin() + nEnd);
			return;
		}
		int Mid = (nStart + nEnd) / 2;
		//std::cout << "ProcessJob:[" << nStart << "   " << nEnd << "]" << std::endl;
		YJobHandleRef LeftHalfSoft = YJob::CreateJob<MergeSortJob>(nullptr,nStart, Mid)->DispatchJob();
		YJobHandleRef RightHalfSoft = YJob::CreateJob<MergeSortJob>(nullptr, Mid, nEnd)->DispatchJob();
		std::vector<YJobHandleRef> MergePrerequisites;
		MergePrerequisites.push_back(LeftHalfSoft);
		MergePrerequisites.push_back(RightHalfSoft);
		//ThisJobHandle->DoNotCompleteUnitl(YJob::CreateJob<YMergeJob>(&MergePrerequisites,nStart, nEnd, Mid)->DispatchJob());
		YJob::CreateJob<YMergeJob>(&MergePrerequisites,nStart, nEnd,Mid)->DispatchJob();
		//ThisJobHandle->DoNotCompleteUnitl(LeftHalfSoft);
	}

	int nStart;
	int nEnd;
};
void MergeParallelWithY()
{
	{
		FScopedEvent WaitForTasks;
		std::vector<YJobHandleRef> WaitList;
		WaitList.push_back(YJob::CreateJob<MergeSortJob>(nullptr, 0,gCount)->DispatchJob());
		YJob::CreateJob<TrigerEventJob>(&WaitList, WaitForTasks.Get())->DispatchJob();
		// waitfor(Root.GFXWaitForTickComplete);
		FPlatformProcess::Sleep(10);
	}
}

void CompareResult()
{
	int cmp = GVector2[0];
	for (int i : GVector2)
	{
		if (cmp <= i)
		{
			cmp = i;
		}
		else
		{
			std::cout << "TaskGraph unmarch" << std::endl;
			break;
		}
	}
	std::cout << "TaskGraph march" << std::endl;
}

void CompareResultY()
{
	int cmp = GVector4[0];
	for (int i : GVector4)
	{
		if (cmp <= i)
		{
			cmp = i;
		}
		else
		{
			std::cout << "unmarch" << std::endl;
			break;
		}
	}
	std::cout << " YTaskGraph march" << std::endl;
}

void QSort()
{
	std::sort(GVector3.begin(), GVector3.end());
}
