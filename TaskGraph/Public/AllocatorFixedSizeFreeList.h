// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <assert.h>


/**
 * Fixed-sized allocator that uses a free list to cache allocations.
 * Initial allocation block can be specified in the ctor to warm the cache.
 * Subsequent allocations are in multiples of the given blocksize.
 *
 * Grow can be called at any time to warm the cache with a single block allocation.
 * Initial allocation will want to be a reasonable guess as to how big the pool might grow.
 * BlockSize should be small enough to cut down on allocations but not overcommit memory.
 *
 * NOTE: Currently there is not way to flush the pool because 
 *       it doesn't track each block's allocation status.
 *
 * Not threadsafe <could be implemented using a ThreadingPolicy template parameter>.
 *
 * Template params:
 *   AllocationSize - size of each allocation (must be at least as large as a pointer)
 *   BlockSize - number of allocations to reserve room for when a new block needs to be allocated.
 */
template<unsigned int AllocationSize, unsigned int BlockSize>
class TAllocatorFixedSizeFreeList
{
public:
	/**
	 * Ctor
	 * 
	 * @param	InitialBlockSize		- number of allocations to warm the cache with
	 */
	TAllocatorFixedSizeFreeList(unsigned int InitialBlockSize=0)
		:FreeList(nullptr)
		,NumAllocated(0)
		,NumLive(0)
	{
		// need enough memory to store pointers for the free list
		static_assert(AllocationSize >= sizeof(FreeListNode), "Allocation size must be large enough to hold pointer.");

		// warm the cache
		Grow(InitialBlockSize);
	}

	/**
	 * Destructor. Can't free memory, so only checks that allocations have been returned.
	 */
	~TAllocatorFixedSizeFreeList()
	{
		// by now all block better have been returned to me
		assert(NumLive == 0); 
		// WRH - 2007/09/14 - Note we are stranding memory here.
		// These pools are meant to be global and never deleted. 
	}

	/**
	 * Allocates one element from the free list. Return it by calling Free.
	 */
	void *Allocate()
	{
		CheckInvariants();
		if (!FreeList)
		{
			assert(NumLive == NumAllocated);
			Grow(BlockSize);
		}
		// grab next free element, updating FreeList pointer
		void *rawMem = (void *)FreeList;
		FreeList = FreeList->NextFreeAllocation;
		++NumLive;
		CheckInvariants();
		return rawMem;
	}

	/**
	 * Returns one element from the free list.
	 * Must have been acquired previously by Allocate.
	 */
	void Free(void *Element)
	{
		CheckInvariants();
		assert(NumLive > 0);
		--NumLive;
		FreeListNode* NewFreeElement = (FreeListNode*)Element;
		NewFreeElement->NextFreeAllocation = FreeList;
		FreeList = NewFreeElement;
		CheckInvariants();
	}

	/**
	 * Get total memory allocated
	 */
	unsigned int GetAllocatedSize() const
	{
		CheckInvariants();
		return NumAllocated * AllocationSize;
	}

	/**
	 * Grows the free list by a specific number of elements. Does one allocation for all elements.
	 * Safe to call at any time to warm the cache with a single block allocation.
	 */
	void Grow(unsigned int NumElements)
	{
		if (NumElements == 0) 
		{
			return;
		}

		// need enough memory to store pointers for the free list
		assert(AllocationSize*NumElements >= sizeof(FreeListNode));

		// allocate a block of memory
		unsigned char* RawMem = (unsigned char*)malloc(AllocationSize * NumElements);
		FreeListNode* NewFreeList = (FreeListNode*)RawMem;
		
		// Chain the block into a list of free list nodes
		for (unsigned int i=0;i<NumElements-1;++i,NewFreeList=NewFreeList->NextFreeAllocation)
		{
			NewFreeList->NextFreeAllocation = (FreeListNode*)(RawMem + (i+1)*AllocationSize);
		}

		// link the last Node to the previous FreeList
		NewFreeList->NextFreeAllocation = FreeList;
		FreeList = (FreeListNode*)RawMem;

		NumAllocated += NumElements;
	}

private:

	struct FreeListNode
	{
		FreeListNode* NextFreeAllocation;
	};

	void CheckInvariants() const
	{
		assert(NumAllocated >= NumLive);
	}

private:

	/** Linked List of free memory blocks. */
	FreeListNode* FreeList;

	/** The number of objects that have been allocated. */
	unsigned int NumAllocated;

	/** The number of objects that are constructed and "out in the wild". */
	unsigned int NumLive;
};
