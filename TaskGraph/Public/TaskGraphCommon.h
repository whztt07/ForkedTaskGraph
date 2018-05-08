#pragma once
#include "WindowsPlatformProcess.h"
class FNoncopyable
{
protected:
	// ensure the class cannot be constructed directly
	FNoncopyable() {}
	// the class should not be used polymorphically
	~FNoncopyable() {}
private:
	FNoncopyable(const FNoncopyable&);
	FNoncopyable& operator=(const FNoncopyable&);
};

static FORCEINLINE bool IsAligned(const volatile void* Ptr, const unsigned int Alignment)
{
	return !(unsigned long long (Ptr) & (Alignment - 1));
}

/**
* Used to declare an untyped array of data with compile-time alignment.
* It needs to use template specialization as the MS_ALIGN and GCC_ALIGN macros require literal parameters.
*/
template<int Size, unsigned int Alignment>
struct TAlignedBytes; // this intentionally won't compile, we don't support the requested alignment

/** Unaligned storage. */
template<int Size>
struct TAlignedBytes<Size, 1>
{
	unsigned char Pad[Size];
};


// C++/CLI doesn't support alignment of native types in managed code, so we enforce that the element
// size is a multiple of the desired alignment

/** A macro that implements TAlignedBytes for a specific alignment. */
#define IMPLEMENT_ALIGNED_STORAGE(Align) \
	template<int Size>        \
struct TAlignedBytes<Size, Align> \
{ \
struct __declspec(align(Align)) TPadding \
{ \
	unsigned char Pad[Size]; \
} ; \
	TPadding Padding; \
};

// Implement TAlignedBytes for these alignments.
IMPLEMENT_ALIGNED_STORAGE(16);
IMPLEMENT_ALIGNED_STORAGE(8);
IMPLEMENT_ALIGNED_STORAGE(4);
IMPLEMENT_ALIGNED_STORAGE(2);

#undef IMPLEMENT_ALIGNED_STORAGE

extern CORE_API bool GIsGameThreadIdInitialized;
extern CORE_API unsigned int GGameThreadId;

__forceinline bool IsInGameThread()
{
	if (GIsGameThreadIdInitialized)
	{
		const unsigned int CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		return CurrentThreadId == GGameThreadId ;
	}
	return true;
}
