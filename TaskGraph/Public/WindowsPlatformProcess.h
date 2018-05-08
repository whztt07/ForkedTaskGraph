// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <windows.h>
#pragma warning(disable : 4251) 
#define PLATFORM_CACHE_LINE_SIZE	128
class FEvent;
class FRunnableThread;


/**
* Windows implementation of the Process OS functions.
**/
struct CORE_API FWindowsPlatformProcess
{
	
	static void Sleep(float Seconds);
	static void SleepNoStats(float Seconds);
	static void SleepInfinite();
	static class FEvent* CreateSynchEvent(bool bIsManualReset = false);
	/**
	* Gets an event from the pool or creates a new one if necessary.
	*
	* @param bIsManualReset Whether the event requires manual reseting or not.
	* @return An event, or nullptr none could be created.
	* @see CreateSynchEvent, ReturnSynchEventToPool
	*/
	static class FEvent* GetSynchEventFromPool(bool bIsManualReset = false);

	/**
	* Returns an event to the pool.
	*
	* @param Event The event to return.
	* @see CreateSynchEvent, GetSynchEventFromPool
	*/
	static void ReturnSynchEventToPool(FEvent* Event);

	/**
	* Creates the platform-specific runnable thread. This should only be called from FRunnableThread::Create.
	*
	* @return The newly created thread
	*/
	static class FRunnableThread* CreateRunnableThread();
	static bool SupportsMultithreading();
	static int NumberOfWorkerThreadsToSpawn();
	static int NumberOfCores();
	static int NumberOfCoresIncludingHyperthreads();
	static void SetThreadAffinityMask(unsigned long long AffinityMask);
};

typedef FWindowsPlatformProcess FPlatformProcess;

struct FGenericPlatformTLS
{
	/**
	* Return false if this is an invalid TLS slot
	* @param SlotIndex the TLS index to check
	* @return true if this looks like a valid slot
	*/
	static __forceinline bool IsValidTlsSlot(unsigned int SlotIndex)
	{
		return SlotIndex != 0xFFFFFFFF;
	}

};
/**
* Windows implementation of the TLS OS functions.
*/
struct CORE_API FWindowsPlatformTLS
	: public FGenericPlatformTLS
{
	/**
	* Returns the currently executing thread's identifier.
	*
	* @return The thread identifier.
	*/
	static __forceinline unsigned int GetCurrentThreadId(void)
	{
		return ::GetCurrentThreadId();
	}

	/**
	* Allocates a thread local store slot.
	*
	* @return The index of the allocated slot.
	*/
	static __forceinline unsigned int AllocTlsSlot(void)
	{
		return ::TlsAlloc();
	}

	/**
	* Sets a value in the specified TLS slot.
	*
	* @param SlotIndex the TLS index to store it in.
	* @param Value the value to store in the slot.
	*/
	static __forceinline void SetTlsValue(unsigned int SlotIndex, void* Value)
	{
		::TlsSetValue(SlotIndex, Value);
	}

	/**
	* Reads the value stored at the specified TLS slot.
	*
	* @param SlotIndex The index of the slot to read.
	* @return The value stored in the slot.
	*/
	static __forceinline void* GetTlsValue(unsigned int SlotIndex)
	{
		return ::TlsGetValue(SlotIndex);
	}

	/**
	* Frees a previously allocated TLS slot
	*
	* @param SlotIndex the TLS index to store it in
	*/
	static __forceinline void FreeTlsSlot(unsigned int SlotIndex)
	{
		::TlsFree(SlotIndex);
	}
};


typedef FWindowsPlatformTLS FPlatformTLS;
