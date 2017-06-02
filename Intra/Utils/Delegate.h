#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Callable.h"
#include "Utils/Debug.h"
#include "Unique.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Utils {

template<typename FuncSignature> class Delegate;
template<typename R, typename... Args> class Delegate<R(Args...)>
{
	Unique<ICallable<R(Args...)>> mCallback;
public:
	forceinline Delegate(null_t=null): mCallback(null) {}

	template<typename T> Delegate(R(*func)(const T&, Args...), const T& params):
		mCallback(new FreeFuncCallable<R(Args...), T>(func, params)) {}

	template<typename T, typename = Meta::EnableIf<
		!Meta::IsFunction<T>::_
	>> Delegate(const T& obj):
		mCallback(new FunctorCallable<R(Args...), T>(obj)) {}

	Delegate(R(*freeFunction)(Args...)):
		mCallback(freeFunction == null? null: new FunctorCallable<R(Args...)>(freeFunction)) {}

	template<typename T> Delegate(const T& obj, R(T::*method)(Args...)):
		mCallback(method == null? null: new ObjectRefMethodCallable<R(Args...), T>(obj, method)) {}

	Delegate(const Delegate& rhs):
		mCallback(rhs == null? null: rhs.mCallback->Clone()) {}

	forceinline Delegate(Delegate&& rhs):
		mCallback(Cpp::Move(rhs.mCallback)) {}

	forceinline R operator()(Args... args) const
	{return mCallback->Call(Cpp::Forward<Args>(args)...);}

	forceinline bool operator==(null_t) const {return mCallback == null;}
	forceinline bool operator!=(null_t) const {return mCallback != null;}
	forceinline bool operator!() const {return operator==(null);}


	Delegate& operator=(const Delegate& rhs)
	{
		if(rhs.mCallback==null) mCallback = null;
		else mCallback = rhs.mCallback->Clone();
		return *this;
	}

	forceinline Delegate& operator=(Delegate&& rhs)
	{
		mCallback = Cpp::Move(rhs.mCallback);
		return *this;
	}

	Unique<ICallable<R(Args...)>> TakeAwayCallable() {return Cpp::Move(mCallback);}
	ICallable<R(Args...)>& Callable() const {return *mCallback;}
	ICallable<R(Args...)>* ReleaseCallable() {return mCallback.Release();}
};



}
using Utils::Delegate;

}

INTRA_WARNING_POP