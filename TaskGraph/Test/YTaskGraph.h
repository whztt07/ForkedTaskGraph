#pragma once
#include <vector>
#include "ScopeLock.h"
#include "WindowsCriticalSection.h"
#include "ThreadSafeCounter.h"
#include <assert.h>

void YTaskGraphTest();

template<typename T>
class ThreadSafeLockPointerArray
{
public:
	ThreadSafeLockPointerArray()
	{

	}

	inline T*  Pop()
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

	inline void Push(T* InObject)
	{
		FScopeLock Lock(&CriticalSection);
		InternalArray.push_back(InObject);
	}

	inline int Num()
	{
		FScopeLock Lock(&CriticalSection);
		return (int)InternalArray.size();
	}
	std::vector<T*> InternalArray;
	FCriticalSection CriticalSection;
};

template<typename T>
class ThreadSafeLockPointerArrayCloseable :public ThreadSafeLockPointerArray<T>
{
public:
	inline bool IsClosed()
	{
		FScopeLock Lock(&CriticalSection);
		return ClosedFlags.GetValue() == 1;
	}
	inline std::vector<T*> GetArrayValueAndClosed()
	{
		std::vector<T*> tmp;
		{
			FScopeLock Lock(&CriticalSection);
			tmp.swap(InternalArray);
			ClosedFlags.Set(1);
		}
		return std::move(tmp);
	}
	inline bool AddIfNotClosed(T* Value)
	{
		FScopeLock Lock(&CriticalSection);
		if (IsClosed())
		{
			return false;
		}
		else
		{
			Push(Value);
			return true;
		}
	}
private:
	FThreadSafeCounter ClosedFlags;
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
	friend class YTaskGraphInterface;
	friend class YTaskGraphInterfaceImplement;
public:
	YJob();
	virtual void Task(int InThreadID) = 0;
	bool AddChildJob(YJob* Child);
	void EndJob();
private:
	FThreadSafeCounter  PrerequistsCounter;
	ThreadSafeLockPointerArrayCloseable<YJob>  SubsequenceJobs;
};


class YTaskGraphInterface
{
public:
	template<typename T, typename ... arg>
	static T* CreateTask(const std::vector<YJob*> *Depeneces = nullptr,arg&& ... args )
	{
		T* NewJob = new T(std::forward<arg>(args)...);
		if (Depeneces)
		{
			for (YJob* ParentJob : *Depeneces)
			{
				if (ParentJob->SubsequenceJobs.AddIfNotClosed(NewJob))
				{
					NewJob->PrerequistsCounter.Increment();
				}
			}
		}
		return NewJob;
	}

	virtual void DispatchJob(YJob* JobToDispatch)=0;
	static YTaskGraphInterface& Get();

	static void Startup(int NumThreads);

};

