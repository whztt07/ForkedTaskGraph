#include "AsycComputeTree.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include "TaskGraphInterfaces.h"
#include "ScopedEvent.h"

AsycComputeTree::AsycComputeTree()
{
}


AsycComputeTree::~AsycComputeTree()
{
}
std::vector<int>   GVector;
std::vector<int>   GVector2;
std::vector<int>   GVector3;

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
int gCount = 10000;
void AllocResource()
{
	GVector.reserve(gCount);
	for (int i = 0; i < gCount; ++i)
	{
		GVector.push_back(rand() % 65535);
	}

	GVector2.assign(GVector.begin(), GVector.end());
	GVector3.assign(GVector.begin(), GVector.end());
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

void CompareResult()
{
	if (memcmp(&GVector[0], &GVector2[0], sizeof(int)*GVector.size()) == 0)
	{
		std::cout << "march" << std::endl;
	}
	else
	{
		std::cout << "unmarch" << std::endl;
	}
}

void QSort()
{
	std::sort(GVector3.begin(), GVector3.end());
}
