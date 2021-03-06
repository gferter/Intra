﻿#include "Range/Special/Unicode.h"

#include "Cpp/Warnings.h"
#include "Cpp/Intrinsics.h"

#include "Range/Mutation/Copy.h"
#include "Container/Sequential/Array.h"
#include "Container/Sequential/String.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

enum: ushort {HalfShift=10};
enum: uint {HalfBase=0x0010000};
enum: ushort {SurrogateHighStart=0xD800, SurrogateHighEnd=0xDBFF, SurrogateLowStart=0xDC00, SurrogateLowEnd=0xDFFF};

const char UTF8::BOM[] = "\xef\xbb\xbf";

static bool isLegalUTF8(const char* source, size_t length, size_t* oBytesRead)
{
	const byte* srcBegin = reinterpret_cast<const byte*>(source);
	const byte* srcPtr = reinterpret_cast<const byte*>(source)+length;
	if(oBytesRead!=null) *oBytesRead = length;
	byte a = 0;
    switch(length)
    {
    default: return false;
	// Everything else falls through when "true"...
	case 4: if((a=(*--srcPtr))<0x80 || a>0xBF) goto illegal_char;
	case 3: if((a=(*--srcPtr))<0x80 || a>0xBF) goto illegal_char;
	case 2: if((a=(*--srcPtr))>0xBF) goto illegal_char;
		switch(*srcBegin)
		{
		//no fall-through in this inner switch
		case 0xE0: if(a<0xA0) goto illegal_char; break;
		case 0xED: if(a>0x9F) goto illegal_char; break;
		case 0xF0: if(a<0x90) goto illegal_char; break;
		case 0xF4: if(a>0x8F) goto illegal_char; break;
		default:   if(a<0x80) goto illegal_char;
		}
	case 1: if(*srcBegin>=0x80 && *srcBegin<0xC2) goto illegal_char;
    }
    return (*srcBegin<=0xF4);
illegal_char:
	if(oBytesRead!=null) *oBytesRead = size_t(srcPtr-srcBegin);
	return false;
}

dchar UTF8::NextChar(size_t* oBytesRead) const
{
	auto src = Text;
	uint ch = 0;
	size_t bytesToRead = UTF8::SequenceBytes(src.First());

	if(bytesToRead>src.Length())
	{
		if(oBytesRead!=null)
			*oBytesRead = src.Length();
		return UTF32::ReplacementChar;
	}

	if(!isLegalUTF8(src.Data(), bytesToRead, oBytesRead))
		return UTF32::ReplacementChar;

	static const uint offsetsFromUTF8[6] = {0, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080};
	for(size_t i=bytesToRead; i>1; i--)
	{
		ch += byte(src.Next());
		ch <<= 6;
	}
	ch += byte(src.Next())-offsetsFromUTF8[bytesToRead-1];

	if( ch>UTF32::MaxLegalChar || (ch>=SurrogateHighStart && ch<=SurrogateLowEnd) )
		ch = UTF32::ReplacementChar;

	if(oBytesRead!=null) *oBytesRead = bytesToRead;
	return ch;
}

dchar UTF8::ReadPrevChar()
{
	auto oldEnd = Text.end();
	if(!popBackChar()) return UTF32::ReplacementChar;
	return UTF8({Text.end(), oldEnd}).NextChar();
}

dchar UTF8::Last() const
{
	auto temp = *this;
	if(!temp.popBackChar()) return UTF32::ReplacementChar;
	return UTF8({temp.Text.end(), Text.end()}).NextChar();
}

void UTF8::PopFirst()
{
	size_t bytesToRead = UTF8::SequenceBytes(Text.First());
	size_t bytesRead = 0;
	if(bytesToRead > Text.Length()) {Text = null; return;}
	if(!isLegalUTF8(Text.Data(), bytesToRead, &bytesRead))
	{
		Text.PopFirstExactly(bytesRead);
		return;
	}
	//ushort bytesToRead; NextChar(&bytesToRead);
	Text.PopFirstExactly(bytesToRead);
}

bool UTF8::popBackChar()
{
	INTRA_DEBUG_ASSERT(!Empty());
	while( ((Text.Last() & 0xFF) >> 6) == 0x2 )
	{
		Text.PopLast();
		if(Text.Empty()) return false;
	}
	return true;
}


DString UTF8::ToUTF32(bool addNullTerminator) const
{
    DString result;
	result.SetLengthUninitialized(Text.Length() + +addNullTerminator);
	Range::CopyTo(*this, result.AsRange());
	if(addNullTerminator) result += dchar(0);
    return result;
}

WString UTF8::ToUTF16(bool addNullTerminator) const
{
	WString result;
	result.Reserve(Text.Length());
    for(UTF8 copy = *this; !copy.Empty(); copy.PopFirst())
    {
		wchar codes[2];
		if(UTF32::CharToUTF16Pair(copy.First(), codes, codes+1))
			result += codes[0], result += codes[1];
		else result += codes[0];
    }
	if(addNullTerminator) result += wchar('\0');
    return result;
}


dchar UTF16::NextChar(size_t* wcharsRead) const
{
	INTRA_DEBUG_ASSERT(!Empty());
	auto src = Text;
	wchar ch = src.First();
	src.PopFirst();

	if(wcharsRead!=null) *wcharsRead = 1;
	if(ch<SurrogateHighStart || ch>SurrogateHighEnd) return ch;
	if(src.Empty()) return UTF32::ReplacementChar;

	if(wcharsRead!=null) *wcharsRead = 2;
	if(src.First()<SurrogateLowStart || src.First()>SurrogateLowEnd) return ch;
	return (uint(ch-SurrogateHighStart) << HalfShift) + uint(src.First()-SurrogateLowStart) + HalfBase;
}

void UTF16::PopFirst()
{
	INTRA_DEBUG_ASSERT(!Empty());
	bool twoElements = (Text.First()>=SurrogateHighStart && Text.First()<=SurrogateHighEnd);
	Text.PopFirst();
	if(twoElements && !Text.Empty()) Text.PopFirst();
}

bool UTF16::popBackChar()
{
	INTRA_DEBUG_ASSERT(!Empty());
	const bool twoElements = (Text.Last() >= SurrogateHighStart && Text.Last() <= SurrogateHighEnd);
	Text.PopLast();
	if(twoElements)
	{
		if(Text.Empty()) return false;
		Text.PopLast();
	}
	return true;
}

dchar UTF16::ReadPrevChar()
{
	if(!popBackChar()) return UTF32::ReplacementChar;
	return NextChar();
}

dchar UTF16::Last() const
{
	auto temp = *this;
	if(!temp.popBackChar()) return UTF32::ReplacementChar;
	return UTF16({temp.Text.end(), Text.end()}).ReadChar();
}


DString UTF16::ToUTF32() const
{
	DString result;
	result.SetLengthUninitialized(Text.Length());
	Range::CopyTo(*this, result.AsRange());
	return result;
}

bool UTF32::CharToUTF16Pair(dchar code, wchar* first, wchar* second)
{
	if(code<=0xFFFF)
	{
		if(code>=SurrogateHighStart && code<=SurrogateLowEnd)
			*first = UTF16::ReplacementChar;
		else
			*first = wchar(code);
		return false;
	}
	
	if(code>UTF32::MaxLegalChar)
	{
		*first = UTF16::ReplacementChar;
		return false;
	}

	enum {halfMask=0x3FF};
	uint ch=code-HalfBase;
	*first = wchar((ch >> HalfShift)+SurrogateHighStart);
	*second = wchar((ch & halfMask)+SurrogateLowStart);
	return true;
}

size_t UTF32::CharToUTF8Sequence(dchar code, char dst[5])
{
	size_t bytesToWrite;
	if(code<0x80) bytesToWrite=1;
	else if(code<0x800) bytesToWrite=2;
	else if(code<0x10000) bytesToWrite=3;
	else if(code<=UTF32::MaxLegalChar) bytesToWrite=4;
	else bytesToWrite=3, code = UTF32::ReplacementChar;

	enum {byteMask=0xBF, byteMark=0x80};
	static const byte firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
	dst[bytesToWrite] = 0;
	byte* ptr = reinterpret_cast<byte*>(dst)+bytesToWrite;
	for(size_t i=bytesToWrite; i>1; i--)
	{
		*--ptr = byte((code | byteMark) & byteMask);
		code >>= 6;
	}
	*--ptr = byte(code | firstByteMark[bytesToWrite]);
	return bytesToWrite;
}

String UTF16::ToUTF8() const
{
    String result;
	result.Reserve( Text.Length()*2 );
	
    for(UTF16 copy = *this; !copy.Empty(); copy.PopFirst())
    {
		char temp[5];
		UTF32::CharToUTF8Sequence(copy.First(), temp);
		result += StringView(temp, C::strlen(temp));
    }
    return result;
}

String UTF32::ToUTF8() const
{
    String result;
	result.Reserve( Length()*2 );
    for(auto ch: *this)
    {
		char temp[5];
		CharToUTF8Sequence(ch, temp);
		result += StringView(temp, C::strlen(temp));
    }
    return result;
}

WString UTF32::ToUTF16(bool addNullTerminator) const
{
	WString result;
	result.Reserve(Length());
    for(auto ch: *this)
    {
    	wchar first, second;
		if(CharToUTF16Pair(ch, &first, &second))
		{
			result += first;
			result += second;
		}
		else result += first;
    }
	if(addNullTerminator) result += wchar('\0');
    return result;
}

bool UTF8::SequenceIsLegal(const char* source, const char* sourceEnd)
{
    const size_t length = UTF8::SequenceBytes(*source);
    return source+length<=sourceEnd &&
		isLegalUTF8(source, length, null);
}

INTRA_WARNING_POP

}
