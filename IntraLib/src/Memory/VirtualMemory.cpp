﻿#include "Memory/VirtualMemory.h"

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace Intra { namespace Memory {

static uint translate_access(Access access)
{
	static const uint accessTable[] =
	{
		PAGE_NOACCESS, PAGE_READONLY, PAGE_WRITECOPY, PAGE_READWRITE,
		PAGE_EXECUTE, PAGE_EXECUTE_READ, PAGE_EXECUTE_WRITECOPY, PAGE_EXECUTE_READWRITE
	};
	INTRA_CHECK_TABLE_SIZE(accessTable, (byte)Access::End);
	return accessTable[(byte)access];
}

AnyPtr VirtualAlloc(size_t bytes, Access access)
{
	return ::VirtualAlloc(null, bytes, access==Access::None? MEM_RESERVE: MEM_COMMIT, translate_access(access));
}

void VirtualFree(void* ptr, size_t size)
{
	(void)size;
	::VirtualFree(ptr, 0, MEM_RELEASE);
}

void VirtualCommit(void* ptr, size_t bytes, Access access)
{
	INTRA_ASSERT(ptr!=null);
	if(access!=Access::None) ::VirtualAlloc(ptr, bytes, MEM_COMMIT, translate_access(access));
	else ::VirtualFree(ptr, bytes, MEM_DECOMMIT);
}

size_t VirtualMemoryPageSize()
{
	static bool inited=false;
	static SYSTEM_INFO si;
	if(inited) return si.dwPageSize;
	GetSystemInfo(&si);
	inited = true;
	return si.dwPageSize;
}

}}

#elif defined(INTRA_PLATFORM_IS_POSIX)
#include <unistd.h>
#include <sys/mman.h>

namespace Intra { namespace Memory {

static uint translate_access(Access access)
{
	static const uint accessTable[] =
	{
		PROT_NONE, PROT_READ, PROT_WRITE, PROT_READ|PROT_WRITE,
		PROT_EXEC, PROT_EXEC|PROT_READ, PROT_EXEC|PROT_WRITE, PROT_EXEC|PROT_READ|PROT_WRITE
	};
	INTRA_CHECK_TABLE_SIZE(accessTable, (byte)Access::End);
	return accessTable[(byte)access];
}

AnyPtr VirtualAlloc(size_t bytes, Access access)
{
    void* ptr = mmap(null, bytes, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
    msync(ptr, bytes, MS_SYNC|MS_INVALIDATE);
    return ptr;
}
 
void VirtualCommit(void* ptr, size_t bytes, Access access)
{
    mmap(ptr, bytes, translate_access(access), MAP_FIXED|MAP_ANON|(access==Access::None? MAP_PRIVATE: MAP_SHARED), -1, 0);
    msync(ptr, bytes, MS_SYNC|MS_INVALIDATE);
}

void VirtualFree(void* ptr, size_t size)
{
    msync(ptr, size, MS_SYNC);
    munmap(ptr, size);
}

size_t VirtualMemoryPageSize()
{
	static size_t pageSize = sysconf(_SC_PAGESIZE);
	return pageSize;
}

}}

#else
#include <stdlib.h>

namespace Intra { namespace Memory {
//TODO: сделать большое выравнивание
AnyPtr VirtualAlloc(size_t bytes, Access access) {(void)(bytes, commit, access); return core::malloc(bytes);}
void VirtualFree(void* ptr, size_t size) {core::free(ptr);}
void VirtualCommit(void* ptr, size_t bytes, Access access) {(void)(ptr, bytes, access);}
size_t VirtualMemoryPageSize() {return sizeof(void*)*2;}

}}

#endif
