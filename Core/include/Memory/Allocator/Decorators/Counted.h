#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/Debug.h"
#include "Meta/Type.h"

namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename A> struct ACounted: A
{
private:
	size_t mCounter=0;
public:
	ACounted() = default;
	ACounted(const ACounted&) = default;
	ACounted(A&& allocator): A(Meta::Move(allocator)) {}
	ACounted(ACounted&& rhs): A(Meta::Move(rhs)), mCounter(rhs.mCounter) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo)
	{
		auto result = A::Allocate(bytes, sourceInfo);
		if(result!=null) mCounter++;
		return result;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr==null) return;
		A::Free(ptr, size);
		mCounter--;
	}

	size_t AllocationCount() const {return mCounter;}

	ACounted& operator=(ACounted&& rhs)
	{
		A::operator=(Meta::Move(rhs));
		mCounter = rhs.mCounter;
		return *this;
	}
};

INTRA_WARNING_POP

}}