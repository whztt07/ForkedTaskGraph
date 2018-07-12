#include "RunnableThread.h"
#include "WindowsPlatformProcess.h"
#include "SingleThreadRunnable.h"
#include "ThreadManager.h"
#include "Runnable.h"
#include "ScopeLock.h"
#include "EventPool.h"
#include "ScopedEvent.h"
#include "TlsAutoCleanup.h"
#include "IQueuedWork.h"
#include "QueuedThreadPool.h"
#include <vector>
#include <algorithm>


/** The global thread pool */
FQueuedThreadPool* GThreadPool = nullptr;

FQueuedThreadPool* GIOThreadPool = nullptr;

//#if WITH_EDITOR
//FQueuedThreadPool* GLargeThreadPool = nullptr;
//#endif

CORE_API bool IsInSlateThread()
{
	// If this explicitly is a slate thread, not just the main thread running slate
	//return GSlateLoadingThreadId != 0 && FPlatformTLS::GetCurrentThreadId() == GSlateLoadingThreadId;
	return false;
}

CORE_API FRunnableThread* GAudioThread = nullptr;

CORE_API bool IsInAudioThread()
{
	// True if this is the audio thread or if there is no audio thread, then if it is the game thread
	//return (GAudioThreadId != 0 && FPlatformTLS::GetCurrentThreadId() == GAudioThreadId) || (GAudioThreadId == 0 && FPlatformTLS::GetCurrentThreadId() == GGameThreadId);
	return false;
}

CORE_API int GIsRenderingThreadSuspended = 0;

CORE_API FRunnableThread* GRenderingThread = nullptr;

CORE_API bool IsInActualRenderingThread()
{
	return GRenderingThread && FPlatformTLS::GetCurrentThreadId() == GRenderingThread->GetThreadID();
}

CORE_API bool IsInRenderingThread()
{
	return !GRenderingThread || GIsRenderingThreadSuspended || (FPlatformTLS::GetCurrentThreadId() == GRenderingThread->GetThreadID());
}

CORE_API bool IsInParallelRenderingThread()
{
	//return !GRenderingThread || GIsRenderingThreadSuspended || (FPlatformTLS::GetCurrentThreadId() != GGameThreadId);
	return false;
}

CORE_API bool IsInRHIThread()
{
	return false;
	//return GRHIThread && FPlatformTLS::GetCurrentThreadId() == GRHIThread->GetThreadID();
}
CORE_API FRunnableThread* GRHIThread = nullptr;
// Fake threads

// Core version of IsInAsyncLoadingThread
static bool IsInAsyncLoadingThreadCoreInternal()
{
	// No async loading in Core
	return false;
}
bool(*IsInAsyncLoadingThread)() = &IsInAsyncLoadingThreadCoreInternal;

/**
 * Fake thread created when multi-threading is disabled.
 */
class FFakeThread : public FRunnableThread
{
	/** Thread Id pool */
	static unsigned int ThreadIdCounter;

	/** Thread is suspended. */
	bool bIsSuspended;

	/** Runnable object associated with this thread. */
	FSingleThreadRunnable* Runnable;

public:

	/** Constructor. */
	FFakeThread()
		: bIsSuspended(false)
		, Runnable(nullptr)
	{
		ThreadID = ThreadIdCounter++;
		// Auto register with single thread manager.
		FThreadManager::Get().AddThread(ThreadID, this);
	}

	/** Virtual destructor. */
	virtual ~FFakeThread()
	{
		// Remove from the manager.
		FThreadManager::Get().RemoveThread(this);
	}

	/** Tick one time per frame. */
	virtual void Tick() override
	{
		if (Runnable && !bIsSuspended)
		{
			Runnable->Tick();
		}
	}

public:

	// FRunnableThread interface

	virtual void SetThreadPriority(EThreadPriority NewPriority) override
	{
		// Not relevant.
	}

	virtual void Suspend(bool bShouldPause) override
	{
		bIsSuspended = bShouldPause;
	}

	virtual bool Kill(bool bShouldWait) override
	{
		FThreadManager::Get().RemoveThread(this);
		return true;
	}

	virtual void WaitForCompletion() override
	{
		FThreadManager::Get().RemoveThread(this);
	}

	virtual bool CreateInternal(FRunnable* InRunnable, const wchar_t* InThreadName,
		unsigned int InStackSize,
		EThreadPriority InThreadPri, unsigned long long InThreadAffinityMask) override

	{
		Runnable = InRunnable->GetSingleThreadInterface();
		if (Runnable)
		{
			InRunnable->Init();
		}		
		return Runnable != nullptr;
	}
};
unsigned int FFakeThread::ThreadIdCounter = 0xffff;


void FThreadManager::AddThread(unsigned int ThreadId, FRunnableThread* Thread)
{
	FScopeLock ThreadsLock(&ThreadsCritical);
	// Some platforms do not support TLS
	if (Threads.find(ThreadId)!= Threads.end())
	{
		Threads.insert(std::make_pair(ThreadId,Thread));
	}
}

void FThreadManager::RemoveThread(FRunnableThread* Thread)
{
	FScopeLock ThreadsLock(&ThreadsCritical);
	for (auto it = Threads.begin(); it != Threads.end();)
	{
		if (it->second  == Thread)
			Threads.erase(it++);
		else
			it++;
	}
}

void FThreadManager::Tick()
{	
	if (!FPlatformProcess::SupportsMultithreading())
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_FSingleThreadManager_Tick);

		FScopeLock ThreadsLock(&ThreadsCritical);

		// Tick all registered threads.
		for (auto & ThreadPair : Threads)
		{
			ThreadPair.second->Tick();
		}
	}
}

const std::wstring& FThreadManager::GetThreadName(unsigned int ThreadId)
{
	static std::wstring NoThreadName;
	FScopeLock ThreadsLock(&ThreadsCritical);
	auto Result = Threads.find(ThreadId);
	if (Result != Threads.end())
	{
		return Result->second->GetThreadName();
	}
	return NoThreadName;
}

FThreadManager& FThreadManager::Get()
{
	static FThreadManager Singleton;
	return Singleton;
}


/*-----------------------------------------------------------------------------
	FEvent, FScopedEvent
-----------------------------------------------------------------------------*/

unsigned int FEvent::EventUniqueId = 0;

//void FEvent::AdvanceStats()
//{
////#if	STATS
////	EventId = FPlatformAtomics::InterlockedAdd( (int*)&EventUniqueId, 1 );
////	EventStartCycles = 0;
////#endif // STATS
//}

void FEvent::WaitForStats()
{
//#if	STATS
//	// Only start counting on the first wait, trigger will "close" the history.
//	if( FThreadStats::IsCollectingData() && EventStartCycles == 0 )
//	{
//		const unsigned long long PacketEventIdAndCycles = ((unsigned long long)EventId << 32) | 0;
//		STAT_ADD_CUSTOMMESSAGE_PTR( STAT_EventWaitWithId, PacketEventIdAndCycles );
//		EventStartCycles = FPlatformTime::Cycles();
//	}
//#endif // STATS
}

void FEvent::TriggerForStats()
{
//#if	STATS
//	// Only add wait-trigger pairs.
//	if( EventStartCycles > 0 && FThreadStats::IsCollectingData() )
//	{
//		const unsigned int EndCycles = FPlatformTime::Cycles();
//		const int DeltaCycles = int( EndCycles - EventStartCycles );
//		const unsigned long long PacketEventIdAndCycles = ((unsigned long long)EventId << 32) | DeltaCycles;
//		STAT_ADD_CUSTOMMESSAGE_PTR( STAT_EventTriggerWithId, PacketEventIdAndCycles );
//
//		AdvanceStats();
//	}
//#endif // STATS
}

void FEvent::ResetForStats()
{
}

FScopedEvent::FScopedEvent()
	: Event(FEventPool<EEventPoolTypes::AutoReset>::Get().GetEventFromPool())
{ }

FScopedEvent::~FScopedEvent()
{
	Event->Wait();
	FEventPool<EEventPoolTypes::AutoReset>::Get().ReturnToPool(Event);
	Event = nullptr;
}

/*-----------------------------------------------------------------------------
	FRunnableThread
-----------------------------------------------------------------------------*/

unsigned int FRunnableThread::RunnableTlsSlot = FRunnableThread::GetTlsSlot();

unsigned int FRunnableThread::GetTlsSlot()
{
	assert( IsInGameThread() );
	unsigned int TlsSlot = FPlatformTLS::AllocTlsSlot();
	assert( FPlatformTLS::IsValidTlsSlot( TlsSlot ) );
	return TlsSlot;
}

FRunnableThread::FRunnableThread()
	: Runnable(nullptr)
	, ThreadInitSyncEvent(nullptr)
	, ThreadAffinityMask(FPlatformAffinity::GetNoAffinityMask())
	, ThreadPriority(TPri_Normal)
	, ThreadID(0)
{
}

FRunnableThread::~FRunnableThread()
{
	//if (!GIsRequestingExit)
	{
		FThreadManager::Get().RemoveThread(this);
	}
}

//FRunnableThread* FRunnableThread::Create(
//	class FRunnable* InRunnable,
//	const TCHAR* ThreadName,
//	bool bAutoDeleteSelf,
//	bool bAutoDeleteRunnable,
//	unsigned int InStackSize,
//	EThreadPriority InThreadPri,
//	unsigned long long InThreadAffinityMask)
//{
//	return Create(InRunnable, ThreadName, InStackSize, InThreadPri, InThreadAffinityMask);
//}

FRunnableThread* FRunnableThread::Create(
	class FRunnable* InRunnable, 
	const wchar_t* ThreadName,
	unsigned int InStackSize,
	EThreadPriority InThreadPri, 
	unsigned long long InThreadAffinityMask)
{
	FRunnableThread* NewThread = nullptr;
	if (FPlatformProcess::SupportsMultithreading())
	{
		assert(InRunnable);
		// Create a new thread object
		NewThread = FPlatformProcess::CreateRunnableThread();
		if (NewThread)
		{
			// Call the thread's create method
			if (NewThread->CreateInternal(InRunnable,ThreadName,InStackSize,InThreadPri,InThreadAffinityMask) == false)
			{
				// We failed to start the thread correctly so clean up
				delete NewThread;
				NewThread = nullptr;
			}
		}
	}
	else if (InRunnable->GetSingleThreadInterface())
	{
		// Create a fake thread when multithreading is disabled.
		NewThread = new FFakeThread();
		if (NewThread->CreateInternal(InRunnable,ThreadName,InStackSize,InThreadPri) == false)
		{
			// We failed to start the thread correctly so clean up
			delete NewThread;
			NewThread = nullptr;
		}
	}

//#if	STATS
//	if( NewThread )
//	{
//		FStartupMessages::Get().AddThreadMetadata( FName( *NewThread->GetThreadName() ), NewThread->GetThreadID() );
//	}
//#endif // STATS

	return NewThread;
}

void FRunnableThread::SetTls()
{
	// Make sure it's called from the owning thread.
	//assert( ThreadID == FPlatformTLS::GetCurrentThreadId() );
	assert( FPlatformTLS::IsValidTlsSlot(RunnableTlsSlot) );
	FPlatformTLS::SetTlsValue( RunnableTlsSlot, this );
}

void FRunnableThread::FreeTls()
{
	// Make sure it's called from the owning thread.
	assert( ThreadID == FPlatformTLS::GetCurrentThreadId() );
	assert( FPlatformTLS::IsValidTlsSlot(RunnableTlsSlot) );
	FPlatformTLS::SetTlsValue( RunnableTlsSlot, nullptr );

	// Delete all FTlsAutoCleanup objects created for this thread.
	for( auto& Instance : TlsInstances )
	{
		delete Instance;
		Instance = nullptr;
	}
}

/*-----------------------------------------------------------------------------
	FQueuedThread
-----------------------------------------------------------------------------*/

/**
 * This is the interface used for all poolable threads. The usage pattern for
 * a poolable thread is different from a regular thread and this interface
 * reflects that. Queued threads spend most of their life cycle idle, waiting
 * for work to do. When signaled they perform a job and then return themselves
 * to their owning pool via a callback and go back to an idle state.
 */
class FQueuedThread
	: public FRunnable
{
protected:

	/** The event that tells the thread there is work to do. */
	FEvent* DoWorkEvent;

	/** If true, the thread should exit. */
	volatile int TimeToDie;

	/** The work this thread is doing. */
	IQueuedWork* volatile QueuedWork;

	/** The pool this thread belongs to. */
	class FQueuedThreadPool* OwningThreadPool;

	/** My Thread  */
	FRunnableThread* Thread;

	/**
	 * The real thread entry point. It waits for work events to be queued. Once
	 * an event is queued, it executes it and goes back to waiting.
	 */
	virtual unsigned int Run() override
	{
		while (!TimeToDie)
		{
			// This will force sending the stats packet from the previous frame.
			//SET_DWORD_STAT( STAT_ThreadPoolDummyCounter, 0 );
			// We need to wait for shorter amount of time
			bool bContinueWaiting = true;
			while( bContinueWaiting )
			{				
				//DECLARE_SCOPE_CYCLE_COUNTER( TEXT( "FQueuedThread::Run.WaitForWork" ), STAT_FQueuedThread_Run_WaitForWork, STATGROUP_ThreadPoolAsyncTasks );
				// Wait for some work to do
				bContinueWaiting = !DoWorkEvent->Wait( 10 );
			}

			IQueuedWork* LocalQueuedWork = QueuedWork;
			QueuedWork = nullptr;
			//FPlatformMisc::MemoryBarrier();
			assert(LocalQueuedWork || TimeToDie); // well you woke me up, where is the job or termination request?
			while (LocalQueuedWork)
			{
				// Tell the object to do the work
				LocalQueuedWork->DoThreadedWork();
				// Let the object cleanup before we remove our ref to it
				LocalQueuedWork = OwningThreadPool->ReturnToPoolOrGetNextJob(this);
			} 
		}
		return 0;
	}

public:

	/** Default constructor **/
	FQueuedThread()
		: DoWorkEvent(nullptr)
		, TimeToDie(0)
		, QueuedWork(nullptr)
		, OwningThreadPool(nullptr)
		, Thread(nullptr)
	{ }

	/**
	 * Creates the thread with the specified stack size and creates the various
	 * events to be able to communicate with it.
	 *
	 * @param InPool The thread pool interface used to place this thread back into the pool of available threads when its work is done
	 * @param InStackSize The size of the stack to create. 0 means use the current thread's stack size
	 * @param ThreadPriority priority of new thread
	 * @return True if the thread and all of its initialization was successful, false otherwise
	 */
	virtual bool Create(class FQueuedThreadPool* InPool,unsigned int InStackSize = 0,EThreadPriority ThreadPriority=TPri_Normal)
	{
		static int PoolThreadIndex = 0;
		std::wstring PoolThreadName = TEXT("PoolThread ");
		PoolThreadName+=PoolThreadIndex ;
		PoolThreadIndex++;

		OwningThreadPool = InPool;
		DoWorkEvent = FPlatformProcess::GetSynchEventFromPool();
		Thread = FRunnableThread::Create(this,PoolThreadName.c_str(), InStackSize, ThreadPriority, FPlatformAffinity::GetPoolThreadMask());
		assert(Thread);
		return true;
	}
	
	/**
	 * Tells the thread to exit. If the caller needs to know when the thread
	 * has exited, it should use the bShouldWait value and tell it how long
	 * to wait before deciding that it is deadlocked and needs to be destroyed.
	 * NOTE: having a thread forcibly destroyed can cause leaks in TLS, etc.
	 *
	 * @return True if the thread exited graceful, false otherwise
	 */
	virtual bool KillThread()
	{
		bool bDidExitOK = true;
		// Tell the thread it needs to die
		FPlatformAtomics::InterlockedExchange(&TimeToDie,1);
		// Trigger the thread so that it will come out of the wait state if
		// it isn't actively doing work
		DoWorkEvent->Trigger();
		// If waiting was specified, wait the amount of time. If that fails,
		// brute force kill that thread. Very bad as that might leak.
		Thread->WaitForCompletion();
		// Clean up the event
		FPlatformProcess::ReturnSynchEventToPool(DoWorkEvent);
		DoWorkEvent = nullptr;
		delete Thread;
		return bDidExitOK;
	}

	/**
	 * Tells the thread there is work to be done. Upon completion, the thread
	 * is responsible for adding itself back into the available pool.
	 *
	 * @param InQueuedWork The queued work to perform
	 */
	void DoWork(IQueuedWork* InQueuedWork)
	{
		//DECLARE_SCOPE_CYCLE_COUNTER( TEXT( "FQueuedThread::DoWork" ), STAT_FQueuedThread_DoWork, STATGROUP_ThreadPoolAsyncTasks );

		assert(QueuedWork == nullptr && "Can't do more than one task at a time");
		// Tell the thread the work to be done
		QueuedWork = InQueuedWork;
		//FPlatformMisc::MemoryBarrier();
		// Tell the thread to wake up and do its job
		DoWorkEvent->Trigger();
	}

};


/**
 * Implementation of a queued thread pool.
 */
class FQueuedThreadPoolBase : public FQueuedThreadPool
{
protected:

	/** The work queue to pull from. */
	std::vector<IQueuedWork*> QueuedWork;
	
	/** The thread pool to dole work out to. */
	std::vector<FQueuedThread*> QueuedThreads;

	/** All threads in the pool. */
	std::vector<FQueuedThread*> AllThreads;

	/** The synchronization object used to protect access to the queued work. */
	FCriticalSection* SynchQueue;

	/** If true, indicates the destruction process has taken place. */
	bool TimeToDie;

public:

	/** Default constructor. */
	FQueuedThreadPoolBase()
		: SynchQueue(nullptr)
		, TimeToDie(0)
	{ }

	/** Virtual destructor (cleans up the synchronization objects). */
	virtual ~FQueuedThreadPoolBase()
	{
		Destroy();
	}

	virtual bool Create(unsigned int InNumQueuedThreads,unsigned int StackSize = (32 * 1024),EThreadPriority ThreadPriority=TPri_Normal) override
	{
		// Make sure we have synch objects
		bool bWasSuccessful = true;
		assert(SynchQueue == nullptr);
		SynchQueue = new FCriticalSection();
		FScopeLock Lock(SynchQueue);
		// Presize the array so there is no extra memory allocated
		assert(QueuedThreads.size() == 0);
		QueuedThreads.clear();

		// Check for stack size override.
		if( OverrideStackSize > StackSize )
		{
			StackSize = OverrideStackSize;
		}

		// Now create each thread and add it to the array
		for (unsigned int Count = 0; Count < InNumQueuedThreads && bWasSuccessful == true; Count++)
		{
			// Create a new queued thread
			FQueuedThread* pThread = new FQueuedThread();
			// Now create the thread and add it if ok
			if (pThread->Create(this,StackSize,ThreadPriority) == true)
			{
				QueuedThreads.push_back(pThread);
				AllThreads.push_back(pThread);
			}
			else
			{
				// Failed to fully create so clean up
				bWasSuccessful = false;
				delete pThread;
			}
		}
		// Destroy any created threads if the full set was not successful
		if (bWasSuccessful == false)
		{
			Destroy();
		}
		return bWasSuccessful;
	}

	virtual void Destroy() override
	{
		if (SynchQueue)
		{
			{
				FScopeLock Lock(SynchQueue);
				TimeToDie = 1;
				//FPlatformMisc::MemoryBarrier();
				// Clean up all queued objects
				for (int Index = 0; Index < QueuedWork.size(); Index++)
				{
					QueuedWork[Index]->Abandon();
				}
				// Empty out the invalid pointers
				QueuedWork.clear();
			}
			// wait for all threads to finish up
			while (1)
			{
				{
					FScopeLock Lock(SynchQueue);
					if (AllThreads.size() == QueuedThreads.size())
					{
						break;
					}
				}
				FPlatformProcess::Sleep(0.0f);
			}
			// Delete all threads
			{
				FScopeLock Lock(SynchQueue);
				// Now tell each thread to die and delete those
				for (int Index = 0; Index < AllThreads.size(); Index++)
				{
					AllThreads[Index]->KillThread();
					delete AllThreads[Index];
				}
				QueuedThreads.clear();
				AllThreads.clear();
			}
			delete SynchQueue;
			SynchQueue = nullptr;
		}
	}

	int GetNumQueuedJobs() const
	{
		// this is a estimate of the number of queued jobs. 
		// no need for thread safe lock as the queuedWork array isn't moved around in memory so unless this class is being destroyed then we don't need to wrory about it
		return (int)QueuedWork.size();
	}
	virtual int GetNumThreads() const 
	{
		return (int)AllThreads.size();
	}
	void AddQueuedWork(IQueuedWork* InQueuedWork) override
	{
		if (TimeToDie)
		{
			InQueuedWork->Abandon();
			return;
		}
		assert(InQueuedWork != nullptr);
		FQueuedThread* Thread = nullptr;
		// Check to see if a thread is available. Make sure no other threads
		// can manipulate the thread pool while we do this.
		assert(SynchQueue);
		FScopeLock sl(SynchQueue);
		if (QueuedThreads.size() > 0)
		{
			// Cycle through all available threads to make sure that stats are up to date.
			//int Index = 0;
			// Grab that thread to use
			//Thread = QueuedThreads[Index];
			// Remove it from the list so no one else grabs it
			//QueuedThreads.RemoveAt(Index);
			
			Thread = QueuedThreads[0];
			QueuedThreads.erase(QueuedThreads.begin());
		}
		// Was there a thread ready?
		if (Thread != nullptr)
		{
			// We have a thread, so tell it to do the work
			Thread->DoWork(InQueuedWork);
		}
		else
		{
			// There were no threads available, queue the work to be done
			// as soon as one does become available
			QueuedWork.push_back(InQueuedWork);
		}
	}

	virtual bool RetractQueuedWork(IQueuedWork* InQueuedWork) override
	{
		if (TimeToDie)
		{
			return false; // no special consideration for this, refuse the retraction and let shutdown proceed
		}
		assert(InQueuedWork != nullptr);
		assert(SynchQueue);
		FScopeLock sl(SynchQueue);
		//return !!QueuedWork.RemoveSingle(InQueuedWork);
		auto Reuslt = std::remove(QueuedWork.begin(), QueuedWork.end(), InQueuedWork);
		bool bFound = (Reuslt != QueuedWork.end());
		QueuedWork.erase(Reuslt);
		return bFound;
	}

	virtual IQueuedWork* ReturnToPoolOrGetNextJob(FQueuedThread* InQueuedThread) override
	{
		assert(InQueuedThread != nullptr);
		IQueuedWork* Work = nullptr;
		// Check to see if there is any work to be done
		FScopeLock sl(SynchQueue);
		if (TimeToDie)
		{
			assert(!QueuedWork.size());  // we better not have anything if we are dying
		}
		if (QueuedWork.size() > 0)
		{
			// Grab the oldest work in the queue. This is slower than
			// getting the most recent but prevents work from being
			// queued and never done
			Work = QueuedWork[0];
			// Remove it from the list so no one else grabs it
			QueuedWork.erase(QueuedWork.begin());
		}
		if (!Work)
		{
			// There was no work to be done, so add the thread to the pool
			QueuedThreads.push_back(InQueuedThread);
		}
		return Work;
	}
};

unsigned int FQueuedThreadPool::OverrideStackSize = 0;

FQueuedThreadPool* FQueuedThreadPool::Allocate()
{
	return new FQueuedThreadPoolBase;
}


/*-----------------------------------------------------------------------------
	FThreadSingletonInitializer
-----------------------------------------------------------------------------*/

//FTlsAutoCleanup* FThreadSingletonInitializer::Get( std::function<FTlsAutoCleanup*()> CreateInstance, unsigned int& TlsSlot )
//{
//	if (TlsSlot == 0xFFFFFFFF)
//	{
//		const unsigned int ThisTlsSlot = FPlatformTLS::AllocTlsSlot();
//		assert(FPlatformTLS::IsValidTlsSlot(ThisTlsSlot));
//		const unsigned int PrevTlsSlot = FPlatformAtomics::InterlockedCompareExchange( (int*)&TlsSlot, (int)ThisTlsSlot, 0xFFFFFFFF );
//		if (PrevTlsSlot != 0xFFFFFFFF)
//		{
//			FPlatformTLS::FreeTlsSlot( ThisTlsSlot );
//		}
//	}
//	FTlsAutoCleanup* ThreadSingleton = (FTlsAutoCleanup*)FPlatformTLS::GetTlsValue( TlsSlot );
//	if( !ThreadSingleton )
//	{
//		ThreadSingleton = CreateInstance();
//		ThreadSingleton->Register();
//		FPlatformTLS::SetTlsValue( TlsSlot, ThreadSingleton );
//	}
//	return ThreadSingleton;
//}

void FTlsAutoCleanup::Register()
{
	FRunnableThread* RunnableThread = FRunnableThread::GetRunnableThread();
	if( RunnableThread )
	{
		RunnableThread->TlsInstances.push_back( this );
	}
}
