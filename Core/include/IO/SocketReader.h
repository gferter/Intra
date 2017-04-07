#pragma once

#include "Socket.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Container/Sequential/Array.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Stream/Operators.h"
#include "Range/Stream/InputStreamMixin.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

//! �������������� ������ ����� ��� ������ �� ������
class SocketReader: public Range::InputStreamMixin<SocketReader, char>
{
public:
	forceinline SocketReader(null_t=null) {}

	forceinline SocketReader(StreamSocket&& socket, size_t bufferSize=4096):
		mSocket(Meta::Move(socket))
	{
		mBuffer.SetCountUninitialized(bufferSize);
		loadBuffer();
	}

	SocketReader(const SocketReader& rhs) = delete;
	SocketReader& operator=(const SocketReader& rhs) = delete;

	forceinline SocketReader(SocketReader&& rhs) {operator=(Meta::Move(rhs));}

	SocketReader& operator=(SocketReader&& rhs)
	{
		mSocket = Meta::Move(rhs.mSocket);
		mBuffer = Meta::Move(rhs.mBuffer);
		mBufferRest = rhs.mBufferRest;
		rhs.mBufferRest = null;
		return *this;
	}

	SocketReader& operator=(null_t)
	{
		mSocket = null;
		mBuffer = null;
		mBufferRest = null;
		return *this;
	}

	forceinline char First() const {return mBufferRest.First();}
	
	forceinline bool Empty() const {return mBufferRest.Empty();}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}
	
	void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		mBufferRest.PopFirst();
		if(!mBufferRest.Empty()) return;
		loadBuffer();
	}

	size_t PopFirstN(size_t maxToPop)
	{
		size_t bytesLeft = maxToPop;
		while(!Empty())
		{
			bytesLeft -= mBufferRest.PopFirstN(bytesLeft);
			if(bytesLeft == 0) break;
			loadBuffer();
		}
		return maxToPop-bytesLeft;
	}

	size_t CopyAdvanceToAdvance(ArrayRange<char>& dst)
	{
		size_t totalBytesRead = Algo::CopyAdvanceToAdvance(mBufferRest, dst);
		if(!mBufferRest.Empty()) return totalBytesRead;

		if(dst.Length() >= mBuffer.Length())
		{
			const size_t bytesRead = mSocket.Read(dst.Data(), dst.Length());
			dst.Begin += bytesRead;
			totalBytesRead += bytesRead;
		}
		loadBuffer();
		totalBytesRead += Algo::CopyAdvanceToAdvance(mBufferRest, dst);
		return totalBytesRead;
	}

	template<typename AR> Meta::EnableIf<
		Range::IsArrayRangeOfExactly<AR, char>::_ && !Meta::IsConst<AR>::_,
	size_t> CopyAdvanceToAdvance(AR& dst)
	{
		ArrayRange<char> dstArr(dst.Data(), dst.Length());
		const size_t result = CopyAdvanceToAdvance(dstArr);
		Range::PopFirstExactly(dst, result);
		return result;
	}

	forceinline ArrayRange<const char> BufferedData() const {return mBufferRest;}

	forceinline StreamSocket& Socket() {return mSocket;}
	forceinline const StreamSocket& Socket() const {return mSocket;}

private:
	void loadBuffer()
	{
		const size_t bytesRead = mSocket.Receive(mBuffer.Data(), mBuffer.Length());
		mBufferRest = mBuffer(0, bytesRead);
	}

	StreamSocket mSocket;
	Array<char> mBuffer;
	ArrayRange<char> mBufferRest;
};

}}

INTRA_WARNING_POP
