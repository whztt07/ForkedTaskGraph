// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include <assert.h>
#include <intrin.h>
#define  PLATFORM_64BITS 1
#define  PLATFORM_HAS_64BIT_ATOMICS 1
#define  PLATFORM_HAS_128BIT_ATOMICS 0
struct FGenericPlatformAtomics
{
	/**
	* @return true, if the processor we are running on can execute compare and exchange 128-bit operation.
	* @see cmpxchg16b, early AMD64 processors don't support this operation.
	*/
	static __forceinline bool CanUseCompareExchange128()
	{
		return false;
	}

protected:
	/**
	* Checks if a pointer is aligned and can be used with atomic functions.
	*
	* @param Ptr - The pointer to check.
	*
	* @return true if the pointer is aligned, false otherwise.
	*/
	static inline bool IsAligned(const volatile void* Ptr, const unsigned int Alignment = sizeof(void*))
	{
		return !(unsigned long long(Ptr) & (Alignment - 1));
	}
};


/**
* Windows implementation of the Atomics OS functions
*/
struct CORE_API FWindowsPlatformAtomics
	: public FGenericPlatformAtomics
{
	static __forceinline int InterlockedIncrement(volatile int* Value)
	{
		return (int)::_InterlockedIncrement((long*)Value);
	}

//#if PLATFORM_HAS_64BIT_ATOMICS
#if 1
	static __forceinline signed long long InterlockedIncrement(volatile signed long long* Value)
	{
#if PLATFORM_64BITS
		return (signed long long)::_InterlockedIncrement64((signed long long*)Value);
#else
		// No explicit instruction for 64-bit atomic increment on 32-bit processors; has to be implemented in terms of CMPXCHG8B
		for (;;)
		{
			signed long long OldValue = *Value;
			if (_InterlockedCompareExchange64(Value, OldValue + 1, OldValue) == OldValue)
			{
				return OldValue + 1;
			}
		}
#endif
	}
#endif

	static __forceinline int InterlockedDecrement(volatile int* Value)
	{
		return (int)::_InterlockedDecrement((long*)Value);
	}

#if PLATFORM_HAS_64BIT_ATOMICS
	static __forceinline signed long long InterlockedDecrement(volatile signed long long* Value)
	{
#if PLATFORM_64BITS
		return (signed long long)::_InterlockedDecrement64((signed long long*)Value);
#else
		// No explicit instruction for 64-bit atomic decrement on 32-bit processors; has to be implemented in terms of CMPXCHG8B
		for (;;)
		{
			signed long long OldValue = *Value;
			if (_InterlockedCompareExchange64(Value, OldValue - 1, OldValue) == OldValue)
			{
				return OldValue - 1;
			}
		}
#endif
	}
#endif

	static __forceinline int InterlockedAdd(volatile int* Value, int Amount)
	{
		return (int)::_InterlockedExchangeAdd((long*)Value, (long)Amount);
	}

#if PLATFORM_HAS_64BIT_ATOMICS
	static __forceinline signed long long InterlockedAdd(volatile signed long long* Value, signed long long Amount)
	{
#if PLATFORM_64BITS
		return (signed long long)::_InterlockedExchangeAdd64((signed long long*)Value, (signed long long)Amount);
#else
		// No explicit instruction for 64-bit atomic add on 32-bit processors; has to be implemented in terms of CMPXCHG8B
		for (;;)
		{
			signed long long OldValue = *Value;
			if (_InterlockedCompareExchange64(Value, OldValue + Amount, OldValue) == OldValue)
			{
				return OldValue + Amount;
			}
		}
#endif
	}
#endif

	static __forceinline int InterlockedExchange(volatile int* Value, int Exchange)
	{
		return (int)::_InterlockedExchange((long*)Value, (long)Exchange);
	}

	static __forceinline signed long long InterlockedExchange(volatile signed long long* Value, signed long long Exchange)
	{
#if PLATFORM_64BITS
		return ::_InterlockedExchange64(Value, Exchange);
#else
		// No explicit instruction for 64-bit atomic exchange on 32-bit processors; has to be implemented in terms of CMPXCHG8B
		for (;;)
		{
			signed long long OldValue = *Value;
			if (_InterlockedCompareExchange64(Value, Exchange, OldValue) == OldValue)
			{
				return OldValue;
			}
		}
#endif
	}

	static __forceinline void* InterlockedExchangePtr(void** Dest, void* Exchange)
	{
#ifdef _DEBUG 
		if (IsAligned(Dest) == false)
		{
			//HandleAtomicsFailure(TEXT("InterlockedExchangePointer requires Dest pointer to be aligned to %d bytes"), sizeof(void*));
			assert(0);
		}
#endif

#if PLATFORM_64BITS
		return (void*)::_InterlockedExchange64((signed long long*)(Dest), (signed long long)(Exchange));
#else
		return (void*)::_InterlockedExchange((long*)(Dest), (long)(Exchange));
#endif
	}

	static __forceinline int InterlockedCompareExchange(volatile int* Dest, int Exchange, int Comparand)
	{
		return (int)::_InterlockedCompareExchange((long*)Dest, (long)Exchange, (long)Comparand);
	}

//#if PLATFORM_HAS_64BIT_ATOMICS
#if 1
	static __forceinline signed long long InterlockedCompareExchange(volatile signed long long* Dest, signed long long Exchange, signed long long Comparand)
	{
#ifdef _DEBUG 
		if (IsAligned(Dest) == false)
		{
			//HandleAtomicsFailure(_T("InterlockedCompareExchangePointer requires Dest pointer to be aligned to %d bytes"), sizeof(void*));
			assert(0);
		}
#endif

		return (signed long long)::_InterlockedCompareExchange64(Dest, Exchange, Comparand);
	}
	static __forceinline signed long long AtomicRead64(volatile const signed long long* Src)
	{
		return InterlockedCompareExchange((volatile signed long long*)Src, 0, 0);
	}

#endif

	/**
	*	The function compares the Destination value with the Comparand value:
	*		- If the Destination value is equal to the Comparand value, the Exchange value is stored in the address specified by Destination,
	*		- Otherwise, the initial value of the Destination parameter is stored in the address specified specified by Comparand.
	*
	*	@return true if Comparand equals the original value of the Destination parameter, or false if Comparand does not equal the original value of the Destination parameter.
	*
	*	Early AMD64 processors lacked the CMPXCHG16B instruction.
	*	To determine whether the processor supports this operation, call the IsProcessorFeaturePresent function with PF_COMPARE64_EXCHANGE128.
	*/
#if	PLATFORM_HAS_128BIT_ATOMICS
	static __forceinline bool InterlockedCompareExchange128(volatile FInt128* Dest, const FInt128& Exchange, FInt128* Comparand)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (IsAligned(Dest, 16) == false)
		{
			HandleAtomicsFailure(TEXT("InterlockedCompareExchangePointer requires Dest pointer to be aligned to 16 bytes"));
		}
		if (IsAligned(Comparand, 16) == false)
		{
			HandleAtomicsFailure(TEXT("InterlockedCompareExchangePointer requires Comparand pointer to be aligned to 16 bytes"));
		}
#endif

		return ::_InterlockedCompareExchange128((signed long long volatile *)Dest, Exchange.High, Exchange.Low, (signed long long*)Comparand) == 1;
	}
	/**
	* Atomic read of 128 bit value with a memory barrier
	*/
	static __forceinline void AtomicRead128(const volatile FInt128* Src, FInt128* OutResult)
	{
		FInt128 Zero;
		Zero.High = 0;
		Zero.Low = 0;
		*OutResult = Zero;
		InterlockedCompareExchange128((volatile FInt128*)Src, Zero, OutResult);
	}

#endif // PLATFORM_HAS_128BIT_ATOMICS

	static __forceinline void* InterlockedCompareExchangePointer(void** Dest, void* Exchange, void* Comparand)
	{
#ifdef _DEBUG 
		if (IsAligned(Dest) == false)
		{
			//HandleAtomicsFailure(TEXT("InterlockedCompareExchangePointer requires Dest pointer to be aligned to %d bytes"), sizeof(void*));
			assert(0);
		}
#endif

#if PLATFORM_64BITS
		return (void*)::_InterlockedCompareExchange64((signed long long volatile *)Dest, (signed long long)Exchange, (signed long long)Comparand);
#else
		return (void*)::_InterlockedCompareExchange((long volatile *)Dest, (long)Exchange, (long)Comparand);
#endif
	}

	/**
	* @return true, if the processor we are running on can execute compare and exchange 128-bit operation.
	* @see cmpxchg16b, early AMD64 processors don't support this operation.
	*/
	static __forceinline bool CanUseCompareExchange128()
	{
		assert(0);
		return false;
		//return !!Windows::IsProcessorFeaturePresent(WINDOWS_PF_COMPARE_EXCHANGE128);
	}

protected:
	/**
	* Handles atomics function failure.
	*
	* Since 'check' has not yet been declared here we need to call external function to use it.
	*
	* @param InFormat - The string format string.
	*/
	//static void HandleAtomicsFailure(const TCHAR* InFormat, ...);
};



typedef FWindowsPlatformAtomics FPlatformAtomics;
