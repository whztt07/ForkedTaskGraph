#pragma once
#include <vector>
#include "ScopeLock.h"
#include "WindowsCriticalSection.h"
#include <assert.h>

void YTaskGraphTest();

template<typename T>
class ThreadSafeLockPointerArray
{
public:
	ThreadSafeLockPointerArray()
	{

	}

	T*  Pop()
	{
		FScopeLock Lock(&CriticalSection);
		if (InternalArray.size() == 0)
		{
			//aseert(0);
			return nullptr;
		}
		else
		{
			T* Last = InternalArray.back();
			InternalArray.pop_back();
			return Last;
		}
	}

	void Push(T* InObject)
	{
		FScopeLock Lock(&CriticalSection);
		InternalArray.push_back(InObject);
	}

	int Num()
	{
		FScopeLock Lock(&CriticalSection);
		return (int)InternalArray.size();
	}
	std::vector<T*> InternalArray;
	FCriticalSection CriticalSection;
};

template<typename T>
class ThreadSafeLockValueArray
{
public:
	ThreadSafeLockValueArray()
	{

	}

	T  Pop()
	{
		FScopeLock Lock(&CriticalSection);
		if (InternalArray.size() == 0)
		{
			aseert(0);
			return T();
		}
		else
		{
			T Last = InternalArray.back();
			InternalArray.pop_back();
			return Last;
		}
	}

	void Push(T InObject)
	{
		FScopeLock Lock(&CriticalSection);
		InternalArray.push_back(InObject);
	}

	int Num()
	{
		FScopeLock Lock(&CriticalSection);
		return (int)InternalArray.size();
	}
	std::vector<T> InternalArray;
	FCriticalSection CriticalSection;
};
class YJob
{
public:
	virtual void Task(int InThreadID) = 0;
};
