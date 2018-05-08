// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Event.h"
#include "WindowsEvent.h"
#include "WindowsPlatformProcess.h"
#include <assert.h>
#include "EventPool.h"
#include <algorithm>
#include <sysinfoapi.h>
#include "WindowsRunnableThread.h"

FEvent* FPlatformProcess::CreateSynchEvent(bool bIsManualReset)
{
	// Allocate the new object
	FEvent* Event = NULL;	
	//if (FPlatformProcess::SupportsMultithreading())
	if (1)
	{
		Event = new FEventWin();
	}
	else
	{
		// Fake event object.
		//Event = new FSingleThreadEvent();
	}
	// If the internal create fails, delete the instance and return NULL
	if (!Event->Create(bIsManualReset))
	{
		delete Event;
		Event = NULL;
	}
	return Event;
}


bool FEventWin::Wait(unsigned int WaitTime, const bool bIgnoreThreadIdleStats /*= false*/)
{
	WaitForStats();

	assert( Event );

	//FThreadIdleStats::FScopeIdle Scope( bIgnoreThreadIdleStats );
	return (WaitForSingleObject( Event, WaitTime ) == WAIT_OBJECT_0);
}

void FEventWin::Trigger()
{
	TriggerForStats();
	assert( Event );
	SetEvent( Event );
}

void FEventWin::Reset()
{
	ResetForStats();
	assert( Event );
	ResetEvent( Event );
}

void FWindowsPlatformProcess::Sleep(float Seconds)
{
	SleepNoStats(Seconds);
}

void FWindowsPlatformProcess::SleepNoStats(float Seconds)
{
	unsigned int Milliseconds = (unsigned int)(Seconds * 1000.0);
	if (Milliseconds == 0)
	{
		::SwitchToThread();
	}
	::Sleep(Milliseconds);
}

void FWindowsPlatformProcess::SleepInfinite()
{
	::Sleep(INFINITE);
}

FEvent* FWindowsPlatformProcess::GetSynchEventFromPool(bool bIsManualReset)
{
	return bIsManualReset
		? FEventPool<EEventPoolTypes::ManualReset>::Get().GetEventFromPool()
		: FEventPool<EEventPoolTypes::AutoReset>::Get().GetEventFromPool();
}


void FWindowsPlatformProcess::ReturnSynchEventToPool(FEvent* Event)
{
	if (!Event)
	{
		return;
	}

	if (Event->IsManualReset())
	{
		FEventPool<EEventPoolTypes::ManualReset>::Get().ReturnToPool(Event);
	}
	else
	{
		FEventPool<EEventPoolTypes::AutoReset>::Get().ReturnToPool(Event);
	}
}

bool FWindowsPlatformProcess::SupportsMultithreading()
{
	return true;
}

int FWindowsPlatformProcess::NumberOfWorkerThreadsToSpawn()
{
	static int MaxGameThreads = 4;
	static int MaxThreads = 16;

	int NumberOfCores = FWindowsPlatformProcess::NumberOfCores();
	//int MaxWorkerThreadsWanted = (IsRunningGame() || IsRunningDedicatedServer() || IsRunningClientOnly()) ? MaxGameThreads : MaxThreads;
	int MaxWorkerThreadsWanted = MaxThreads;
	// need to spawn at least one worker thread (see FTaskGraphImplementation)
	return (std::max)((std::min)(NumberOfCores - 1, MaxWorkerThreadsWanted), 1);
}

int FWindowsPlatformProcess::NumberOfCores()
{
	static int CoreCount = 0;
	if (CoreCount == 0)
	{
			// Get only physical cores
			PSYSTEM_LOGICAL_PROCESSOR_INFORMATION InfoBuffer = NULL;
			::DWORD BufferSize = 0;

			// Get the size of the buffer to hold processor information.
			::BOOL Result = GetLogicalProcessorInformation(InfoBuffer, &BufferSize);
			assert(!Result && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
			assert(BufferSize > 0);

			// Allocate the buffer to hold the processor info.
			InfoBuffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(BufferSize);
			assert(InfoBuffer);

			// Get the actual information.
			Result = GetLogicalProcessorInformation(InfoBuffer, &BufferSize);
			assert(Result);

			// Count physical cores
			const int InfoCount = (int)(BufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
			for (int Index = 0; Index < InfoCount; ++Index)
			{
				SYSTEM_LOGICAL_PROCESSOR_INFORMATION* Info = &InfoBuffer[Index];
				if (Info->Relationship == RelationProcessorCore)
				{
					CoreCount++;
				}
			}
			free(InfoBuffer);
	}
	return CoreCount;
}

int FWindowsPlatformProcess::NumberOfCoresIncludingHyperthreads()
{
	static int CoreCount = 0;
	if (CoreCount == 0)
	{
		// Get the number of logical processors, including hyperthreaded ones.
		SYSTEM_INFO SI;
		GetSystemInfo(&SI);
		CoreCount = (int)SI.dwNumberOfProcessors;
	}
	return CoreCount;
}

void FWindowsPlatformProcess::SetThreadAffinityMask(unsigned long long AffinityMask)
{
	if (AffinityMask != FPlatformAffinity::GetNoAffinityMask())
	{
		::SetThreadAffinityMask(::GetCurrentThread(), (DWORD_PTR)AffinityMask);
	}
}

FRunnableThread* FWindowsPlatformProcess::CreateRunnableThread()
{
	return new FRunnableThreadWin();
}
