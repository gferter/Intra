#include "Concurrency/Mutex.h"

#include <pthread.h>

#ifdef _MSC_VER
#pragma comment(lib, "pthread.lib")
#endif

namespace Intra { namespace Concurrency {

static void createMutex(void* mutexData, bool recursive)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, recursive? PTHREAD_MUTEX_RECURSIVE: PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(static_cast<pthread_mutex_t*>(mutexData), &attr);
}

Mutex::Mutex()
{
	static_assert(sizeof(mData) >= sizeof(pthread_mutex_t), "Invalid DATA_SIZE in Mutex.h!");
	createMutex(mData, false);
}
Mutex::~Mutex() {pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(mData));}
void Mutex::Lock() {pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(mData));}
bool Mutex::TryLock() {return pthread_mutex_trylock(reinterpret_cast<pthread_mutex_t*>(mData)) != 0;}
void Mutex::Unlock() {pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(mData));}


RecursiveMutex::RecursiveMutex()
{
	static_assert(sizeof(mData) >= sizeof(pthread_mutex_t), "Invalid DATA_SIZE in Mutex.h!");
	createMutex(mData, true);
}
RecursiveMutex::~RecursiveMutex() {pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(mData));}
void RecursiveMutex::Lock() {pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(mData));}
bool RecursiveMutex::TryLock() {return pthread_mutex_trylock(reinterpret_cast<pthread_mutex_t*>(mData)) != 0;}
void RecursiveMutex::Unlock() {pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(mData));}

}}
