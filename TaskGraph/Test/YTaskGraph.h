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
	ThreadSafeLockPointerArray(){}

	inline T*  Pop()
	{
		FScopeLock Lock(&CriticalSection);
		if (InternalArray.size() == 0)
		{
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

	inline size_t Num()
	{
		FScopeLock Lock(&CriticalSection);
		return InternalArray.size();
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
			FScopeLock Lock(&CriticalSection);
			std::vector<T*> tmp;
			if (ClosedFlags.GetValue() == 1)
			{
				return tmp;
			}
			tmp.swap(InternalArray);
			ClosedFlags.Set(1);
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

	size_t Num()
	{
		FScopeLock Lock(&CriticalSection);
		return InternalArray.size();
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
	static FThreadSafeCounter&  GetClassCount() { return DebugForMemoryLose; }
private:
	YJobHandle() { /*DebugForMemoryLose.Increment();*/ };
	~YJobHandle() {/* DebugForMemoryLose.Decrement();*/ };
	bool AddChildJob(YJob* Child);
	std::vector<YJobHandleRef>		   WaitForJobs;
	FThreadSafeCounter ReferenceCount;
	ThreadSafeLockPointerArrayCloseable<YJob>  SubsequenceJobs;
	static FThreadSafeCounter   DebugForMemoryLose;
};

class YJob
{
	friend class YTaskGraphInterface;
	friend class YTaskGraphInterfaceImplement;
public:
	YJob() { PrerequistsCounter.Set(1); };
	virtual ~YJob() {};
	virtual void Task(int InThreadID, const YJobHandleRef& ThisJobHandle) = 0;
	
	template<typename T, typename ... args>
	static T* CreateJob(YJobHandleRef JobRef, const std::vector<YJobHandleRef> *Depeneces = nullptr, args&& ... arg)
	{
		T* NewJob = new T(std::forward<args>(arg)...);
		NewJob->JobHandle.Swap(JobRef);
		int nWaitFor = 0;
		if (Depeneces)
		{
			NewJob->PrerequistsCounter.Add((int)Depeneces->size());
			for (YJobHandleRef ParentJob : *Depeneces)
			{
				if (!ParentJob->SubsequenceJobs.AddIfNotClosed(NewJob))
				{
					++nWaitFor;
				}
			}
			NewJob->PrerequistsCounter.Subtract(nWaitFor);
		}
		
		return NewJob;
	}

	template<typename T, typename ... args>
	static T* CreateJob(const std::vector<YJobHandleRef> *Depeneces = nullptr, args&& ... arg)
	{
		return CreateJob<T>(YJobHandleRef(new YJobHandle), Depeneces, std::forward<args>(arg)...);
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
	YJobNullTask() {};

	virtual void Task(int InThreadID, const YJobHandleRef&) override {};
};

class TrigerEventJob :public YJob
{
public:
	TrigerEventJob(FEvent* pEvent)
	{
		Event = pEvent;
	}
	virtual void Task(int InThreadIDY, const YJobHandleRef& ThisJobHandle) override;
	
	FEvent* Event;
};