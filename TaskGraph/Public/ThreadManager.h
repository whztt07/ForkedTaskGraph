// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "WindowsCriticalSection.h"
#include "map"

/**
 * Manages runnables and runnable threads.
 */
class CORE_API FThreadManager
{
	/** List of thread objects to be ticked. */
	std::map<unsigned int, class FRunnableThread*> Threads;
	/** Critical section for ThreadList */
	FCriticalSection ThreadsCritical;

public:

	/**
	* Used internally to add a new thread object.
	*
	* @param Thread thread object.
	* @see RemoveThread
	*/
	void AddThread(unsigned int ThreadId, class FRunnableThread* Thread);

	/**
	* Used internally to remove thread object.
	*
	* @param Thread thread object to be removed.
	* @see AddThread
	*/
	void RemoveThread(class FRunnableThread* Thread);

	/** Ticks all fake threads and their runnable objects. */
	void Tick();

	/** Returns the name of a thread given its TLS id */
	const std::wstring& GetThreadName(unsigned int ThreadId);

	/**
	 * Access to the singleton object.
	 *
	 * @return Thread manager object.
	 */
	static FThreadManager& Get();
};
