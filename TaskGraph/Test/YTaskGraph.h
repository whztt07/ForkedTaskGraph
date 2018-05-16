#pragma once
#include <vector>
#include "ScopeLock.h"
#include "WindowsCriticalSection.h"
#include "ThreadSafeCounter.h"
#include "RefCounting.h"
#include <assert.h>
#include "Event.h"

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

class YJob;

typedef TRefCountPtr<class YJobHandle> YJobHandleRef;
class YJobHandle
{
public:
	friend class YTaskGraphInterface;
	friend class YTaskGraphInterfaceImplement;
	friend class YJob;
	unsigned int AddRef()
	{
		int RefCount = ReferenceCount.Increment();
		assert(RefCount > 0);
		return RefCount;
	}
	unsigned int Release()
	{
		int RefCount = ReferenceCount.Decrement();
		assert(RefCount >= 0);
		if (RefCount == 0)
		{
			delete this;
		}
		return RefCount;
	}
	
	void DoNotCompleteUnitl(YJobHandleRef JobHandleToWaitFor);
	void DispatchSubsequents();
	bool IsComplelte() ;
private:
	YJobHandle();
	~YJobHandle();
	bool AddChildJob(YJob* Child);
	std::vector<YJobHandleRef>		   WaitForJobs;
	FThreadSafeCounter ReferenceCount;
	ThreadSafeLockPointerArrayCloseable<YJob>  SubsequenceJobs;
};

class YJob
{
	friend class YTaskGraphInterface;
	friend class YTaskGraphInterfaceImplement;
public:
	YJob();
	virtual void Task(int InThreadID, YJobHandleRef ThisJobHandle) = 0;
	template<typename T,typename ... args>
	static T* CreateJob(const std::vector<YJobHandleRef> *Depeneces = nullptr, args&& ... arg)
	{
		T* NewJob = new T(std::forward<args>(arg)...);
		if (Depeneces)
		{
			for (YJobHandleRef ParentJob : *Depeneces)
			{
				if (ParentJob->SubsequenceJobs.AddIfNotClosed(NewJob))
				{
					NewJob->PrerequistsCounter.Increment();
				}
			}
		}
		NewJob->JobHandle = new YJobHandle();
		return NewJob;
	}
	template<typename T, typename ... args>
	static T* CreateJob(YJobHandleRef JobRef, std::vector<YJobHandleRef> *Depeneces = nullptr, args&& ... arg)
	{
		T* NewJob = new T(std::forward<args>(arg)...);
		if (Depeneces)
		{
			for (YJobHandleRef ParentJob : *Depeneces)
			{
				if (ParentJob->SubsequenceJobs.AddIfNotClosed(NewJob))
				{
					NewJob->PrerequistsCounter.Increment();
				}
			}
		}
		NewJob->JobHandle.Swap(JobRef);
		return NewJob;
	}
	YJobHandleRef DispatchJob();
	YJobHandleRef GetJobHandle();
	void ExcuteTask(int InThreadI);
private:
	FThreadSafeCounter  PrerequistsCounter;
	YJobHandleRef       JobHandle;
};

class YTaskGraphInterface
{
public:

	virtual void DispatchJob(YJob* JobToDispatch)=0;
	static YTaskGraphInterface& Get();

	static void Startup(int NumThreads);

};

class YJobNullTask:public YJob
{
public:
	YJobNullTask();
	virtual void Task(int InThreadID,YJobHandleRef) override;
};

class TrigerEventJob :public YJob
{
public:
	TrigerEventJob(FEvent* pEvent)
	{
		Event = pEvent;
	}
	virtual void Task(int InThreadIDY, YJobHandleRef ThisJobHandle) override;
	
	FEvent* Event;
};