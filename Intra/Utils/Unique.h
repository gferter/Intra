﻿#pragma once

#include "Cpp/Fundamental.h"
#include "Utils/Debug.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace Utils {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct Unique
{
	forceinline Unique(null_t=null) noexcept: mPtr(null) {}

	//! Этот конструктор предполагает, что объект выделен как new T(...).
	//! Передавать другие указатели запрещено.
	//! Объект по переданному указателю переходит во владение
	//! нового экземпляра Unique и освобождается в деструкторе, используя delete.
	forceinline Unique(T* ptrFromNew) noexcept: mPtr(ptrFromNew) {}

	forceinline ~Unique() noexcept {delete mPtr;}

	forceinline Unique(const Unique& rhs) = delete;
	Unique& operator=(const Unique& rhs) = delete;

	forceinline Unique(Unique&& rhs) noexcept:
		mPtr(rhs.mPtr) {rhs.mPtr = null;}

	template<typename... Args> forceinline static Unique New(Args&&... args) noexcept
	{return new T(Cpp::Forward<Args>(args)...);}

	Unique& operator=(Unique&& rhs) noexcept
	{
		if(this == &rhs) return *this;
		delete mPtr;
		mPtr = rhs.mPtr;
		rhs.mPtr = null;
		return *this;
	}

	Unique& operator=(T* ptrFromNew) noexcept
	{
		if(mPtr == ptrFromNew) return *this;
		delete mPtr;
		mPtr = ptrFromNew;
		return *this;
	}

	forceinline Unique& operator=(null_t) noexcept
	{
		delete mPtr;
		mPtr = null;
		return *this;
	}

	forceinline T* Ptr() const noexcept {return mPtr;}
	forceinline T* get() const noexcept {return mPtr;}

	forceinline T* Release()
	{
		T* const result = mPtr;
		mPtr = null;
		return result;
	}

	forceinline T& operator*() const {INTRA_DEBUG_ASSERT(mPtr!=null); return *mPtr;}
	forceinline T* operator->() const {INTRA_DEBUG_ASSERT(mPtr!=null); return mPtr;}

	forceinline bool operator==(null_t) const {return mPtr==null;}
	forceinline bool operator!=(null_t) const {return !operator==(null);}

	forceinline explicit operator bool() const {return mPtr != null;}

private:
	T* mPtr;
};

INTRA_WARNING_POP

template<typename T> forceinline Unique<T> UniqueMove(T& rhs) {return new T(Cpp::Move(rhs));}

}
using Utils::Unique;
using Utils::UniqueMove;

}
