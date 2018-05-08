// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <assert.h>

/**
 * The base class of reference counted objects.
 */
class CORE_API FRefCountedObject
{
public:
	FRefCountedObject(): NumRefs(0) {}
	virtual ~FRefCountedObject() { assert(!NumRefs); }
	unsigned int AddRef() const
	{
		return unsigned int(++NumRefs);
	}
	unsigned int Release() const
	{
		unsigned int Refs = unsigned int(--NumRefs);
		if(Refs == 0)
		{
			delete this;
		}
		return Refs;
	}
	unsigned int GetRefCount() const
	{
		return unsigned int(NumRefs);
	}
private:
	mutable int NumRefs;
};


/**
 * A smart pointer to an object which implements AddRef/Release.
 */
template<typename ReferencedType>
class TRefCountPtr
{
	typedef ReferencedType* ReferenceType;

public:

	__forceinline TRefCountPtr():
		Reference(nullptr)
	{ }

	TRefCountPtr(ReferencedType* InReference,bool bAddRef = true)
	{
		Reference = InReference;
		if(Reference && bAddRef)
		{
			Reference->AddRef();
		}
	}

	TRefCountPtr(const TRefCountPtr& Copy)
	{
		Reference = Copy.Reference;
		if(Reference)
		{
			Reference->AddRef();
		}
	}

	__forceinline TRefCountPtr(TRefCountPtr&& Copy)
	{
		Reference = Copy.Reference;
		Copy.Reference = nullptr;
	}

	~TRefCountPtr()
	{
		if(Reference)
		{
			Reference->Release();
		}
	}

	TRefCountPtr& operator=(ReferencedType* InReference)
	{
		// Call AddRef before Release, in case the new reference is the same as the old reference.
		ReferencedType* OldReference = Reference;
		Reference = InReference;
		if(Reference)
		{
			Reference->AddRef();
		}
		if(OldReference)
		{
			OldReference->Release();
		}
		return *this;
	}

	__forceinline TRefCountPtr& operator=(const TRefCountPtr& InPtr)
	{
		return *this = InPtr.Reference;
	}

	TRefCountPtr& operator=(TRefCountPtr&& InPtr)
	{
		if (this != &InPtr)
		{
			ReferencedType* OldReference = Reference;
			Reference = InPtr.Reference;
			InPtr.Reference = nullptr;
			if(OldReference)
			{
				OldReference->Release();
			}
		}
		return *this;
	}

	__forceinline ReferencedType* operator->() const
	{
		return Reference;
	}

	__forceinline operator ReferenceType() const
	{
		return Reference;
	}

	__forceinline ReferencedType** GetInitReference()
	{
		*this = nullptr;
		return &Reference;
	}

	__forceinline ReferencedType* GetReference() const
	{
		return Reference;
	}

	__forceinline friend bool IsValidRef(const TRefCountPtr& InReference)
	{
		return InReference.Reference != nullptr;
	}

	__forceinline bool IsValid() const
	{
		return Reference != nullptr;
	}

	__forceinline void SafeRelease()
	{
		*this = nullptr;
	}

	unsigned int GetRefCount()
	{
		unsigned int Result = 0;
		if (Reference)
		{
			Result = Reference->GetRefCount();
			assert(Result > 0); // you should never have a zero ref count if there is a live ref counted pointer (*this is live)
		}
		return Result;
	}

	__forceinline void Swap(TRefCountPtr& InPtr) // this does not change the reference count, and so is faster
	{
		ReferencedType* OldReference = Reference;
		Reference = InPtr.Reference;
		InPtr.Reference = OldReference;
	}
private:

	ReferencedType* Reference;
};

template<typename ReferencedType>
__forceinline bool operator==(const TRefCountPtr<ReferencedType>& A, const TRefCountPtr<ReferencedType>& B)
{
	return A.GetReference() == B.GetReference();
}

template<typename ReferencedType>
__forceinline bool operator==(const TRefCountPtr<ReferencedType>& A, ReferencedType* B)
{
	return A.GetReference() == B;
}

template<typename ReferencedType>
__forceinline bool operator==(ReferencedType* A, const TRefCountPtr<ReferencedType>& B)
{
	return A == B.GetReference();
}
