#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

#ifdef _MSC_VER
#pragma warning(disable: 4512 4626)

#if _MSC_VER >= 1900
#pragma warning(disable: 5026 5027)
#endif

#endif

namespace Intra { namespace Meta {

INTRA_DEFINE_EXPRESSION_CHECKER(Has_first_second, (Val<T>().first, Val<T>().second));
INTRA_DEFINE_EXPRESSION_CHECKER(HasKeyValue, (Val<T>().Key, Val<T>().Value));

template<typename T1, typename T2 = T1> struct Pair
{
	T1 first;
	T2 second;
};

template<typename T> struct Pair<T, T>
{
	T first, second;
	T& operator[](size_t i) {return i? second: first;}
	const T& operator[](size_t i) const {return i? second: first;}
};

template<typename K, typename V> struct KeyValuePair
{
	K Key;
	V Value;

	KeyValuePair(): Key(), Value() {};
	template<typename K1, typename V1> KeyValuePair(K1&& key, V1&& value):
		Key(Cpp::Forward<K1>(key)), Value(Cpp::Forward<V1>(value)) {}

	forceinline operator KeyValuePair<const K, V>() const
	{return *reinterpret_cast<KeyValuePair<const K, V>*>(this);}

	forceinline operator Pair<K,V>&() {return *reinterpret_cast<Meta::Pair<K,V>*>(this);}
	forceinline operator const Pair<K,V>&() const {return *reinterpret_cast<Pair<K,V>*>(this);}
};



template<typename T1, typename T2> Pair<T1, T2> PairL(T1&& first, T2&& second)
{return {Cpp::Forward<T1>(first), Cpp::Forward<T2>(second)};}

template<typename K, typename V> KeyValuePair<K, V> KVPairL(K&& key, V&& value)
{return {Cpp::Forward<K>(key), Cpp::Forward<V>(value)};}

}

using Meta::Pair;
using Meta::KeyValuePair;

}

INTRA_WARNING_POP

