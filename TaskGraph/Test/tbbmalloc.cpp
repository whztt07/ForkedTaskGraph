#include <tbb/scalable_allocator.h>

inline void * TBBMalloc(size_t Size)
{
	void* NewPtr = nullptr;
	NewPtr = scalable_malloc(Size);
	return NewPtr;
}
inline void TBBFree(void* Ptr)
{
	if (!Ptr)
	{
		return;
	}
	scalable_free(Ptr);
}

 void* operator new  (size_t Size)  { return TBBMalloc(Size); } 
 void* operator new[](size_t Size) { return TBBMalloc(Size); } 
 void* operator new  (size_t Size, const std::nothrow_t&) throw() { return TBBMalloc(Size); } 
 void* operator new[](size_t Size, const std::nothrow_t&) throw(){ return TBBMalloc(Size); } 
void operator delete  (void* Ptr)                                                  { TBBFree(Ptr); } 
void operator delete[](void* Ptr)                                                 { TBBFree(Ptr); } 
void operator delete  (void* Ptr, const std::nothrow_t&)                          throw() { TBBFree(Ptr); } 
void operator delete[](void* Ptr, const std::nothrow_t&)                          throw() { TBBFree(Ptr); }