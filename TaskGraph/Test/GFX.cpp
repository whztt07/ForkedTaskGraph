#include "GFX.h"
#include <iostream>
#include <string>

int GGameThreadID = FPlatformTLS::GetCurrentThreadId();

void GFX::Tick()
{

	

	struct GFXTickCallBack
	{
		GFXTickCallBack(GFX* pGFX)
		{
			pMainGFX = pGFX;
		}
		static ENamedThreads::Type GetDesiredThread()
		{
			return ENamedThreads::GameThread;
		}
		static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }
		void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
		{
			assert(FPlatformTLS::GetCurrentThreadId() == GGameThreadID);
			assert(CurrentThread == ENamedThreads::GameThread);
			std::cout << "gfx[" << pMainGFX->Name << "] call back is  called " << std::endl;
		}

		GFX* pMainGFX;
	};

	struct GFXTickTask
	{
		GFXTickTask(GFX* pGFX)
		{
			pMainGFX = pGFX;
		}
		static ENamedThreads::Type GetDesiredThread()
		{
			return ENamedThreads::AnyThread;
		}
		static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }
		void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
		{
			assert(pMainGFX);
			std::cout << "gfx[" << pMainGFX->Name << "] is tick" << std::endl;
			if (pMainGFX->ParentGFX)
			{
				pMainGFX->Pos = pMainGFX->ParentGFX->Pos + 1;
			}

			FGraphEventRef WaitCallBack=  TGraphTask<GFXTickCallBack>::CreateTask(NULL, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(pMainGFX);
			MyCompletionGraphEvent->DontCompleteUntil(WaitCallBack);
			for (GFX* pChild : pMainGFX->Children)
			{
				MyCompletionGraphEvent->DontCompleteUntil(TGraphTask<GFXTickTask>::CreateTask(nullptr, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(pChild));
			}
		}

		GFX* pMainGFX;
	};

	GFXWaitForTickComplete = TGraphTask<GFXTickTask>::CreateTask(NULL, ENamedThreads::AnyThread).ConstructAndDispatchWhenReady(this);

}

void GFX::CallBackInGameThread()
{
	
}

//void GFX::Tick()
//{
//	std::function<void(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)> merge = [this,merge](ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
//	{
//		std::cout << "GFX[" << Name << "] is tick" << std::endl;
//		if (ParentGFX)
//		{
//			Pos = ParentGFX->Pos + 1;
//		}
//
//		for (GFX* pChild : Children)
//		{
//			MyCompletionGraphEvent->DontCompleteUntil(EnqueueUniqueRenderCommand(merge));
//		}
//	};
//	GFXWaitForTickComplete = EnqueueUniqueRenderCommand((merge));
//}

//void GFX::GFXTickTask::DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
//{
//	
//}