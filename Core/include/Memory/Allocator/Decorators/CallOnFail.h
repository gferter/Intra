#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/CppWarnings.h"
#include "Platform/Debug.h"
#include "Range/Generators/StringView.h"
#include "Range/Stream.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Memory {

inline void NoMemoryBreakpoint(size_t bytes, SourceInfo sourceInfo)
{
	(void)bytes; (void)sourceInfo;
	INTRA_DEBUGGER_BREAKPOINT;
}

inline void NoMemoryAbort(size_t bytes, SourceInfo sourceInfo)
{
	char errorMsg[512];
	GenericStringView<char>(errorMsg) << StringView(sourceInfo.file).Tail(400) << '(' << sourceInfo.line << "): " <<
		"Out of memory!\n" << "Couldn't allocate " << bytes << " byte memory block." << '\0';
	INTRA_INTERNAL_ERROR(errorMsg);
}

template<typename A, void(*F)(size_t, SourceInfo) = NoMemoryBreakpoint> struct ACallOnFail: A
{
	ACallOnFail() = default;
	ACallOnFail(const ACallOnFail&) = default;
	ACallOnFail(A&& allocator): A(Meta::Move(allocator)) {}
	ACallOnFail(ACallOnFail&& rhs): A(Meta::Move(rhs)) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo)
	{
		auto result = A::Allocate(bytes, sourceInfo);
		if(result==null) F(bytes, sourceInfo);
		return result;
	}

	ACallOnFail& operator=(ACallOnFail&& rhs)
	{
		A::operator=(Meta::Move(rhs));
		return *this;
	}
};

}}

INTRA_WARNING_POP