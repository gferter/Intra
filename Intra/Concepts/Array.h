#pragma once

#include "Cpp/InitializerList.h"
#include "Meta/Type.h"

namespace Intra { namespace Concepts {

INTRA_DEFINE_EXPRESSION_CHECKER(Has_size, static_cast<size_t>(Meta::Val<T>().size()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasLength, static_cast<size_t>(Meta::Val<T>().Length()));
INTRA_DEFINE_EXPRESSION_CHECKER(Has_data, static_cast<const void*>(Meta::Val<T>().data()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasData, static_cast<const void*>(Meta::Val<T>().Data()));
INTRA_DEFINE_EXPRESSION_CHECKER(Has_begin_end, begin(Meta::Val<T>()) != end(Meta::Val<T>()));

namespace D {
template<typename R, bool = HasData<R>::_> struct ReturnTypeOfData {typedef void _;};
template<typename R> struct ReturnTypeOfData<R, true> {typedef decltype(Meta::Val<R>().Data()) _;};

template<typename R, bool = HasData<R>::_> struct ReturnTypeOfDataOrDisable;
template<typename R> struct ReturnTypeOfDataOrDisable<R, true> {typedef decltype(Meta::Val<R>().Data()) _;};

template<typename R, bool = Has_data<R>::_> struct ReturnTypeOf_data {typedef void _;};
template<typename R> struct ReturnTypeOf_data<R, true> {typedef decltype(Meta::Val<R>().data()) _;};

template<typename R, bool = Has_data<R>::_> struct ReturnTypeOf_data_OrDisable;
template<typename R> struct ReturnTypeOf_data_OrDisable<R, true> {typedef decltype(Meta::Val<R>().data()) _;};

}

template<typename T> using ReturnTypeOfData = typename D::ReturnTypeOfData<T>::_;
template<typename T> using ReturnTypeOfDataOrDisable = typename D::ReturnTypeOfDataOrDisable<T>::_;
template<typename T> using ReturnTypeOf_data = typename D::ReturnTypeOf_data<T>::_;
template<typename T> using ReturnTypeOf_data_OrDisable = typename D::ReturnTypeOf_data_OrDisable<T>::_;

template<typename R> forceinline Meta::EnableIf<
	HasLength<R>::_,
size_t> LengthOf(const R& list) {return list.Length();}

template<typename R> forceinline Meta::EnableIf<
	Has_size<R>::_ && !HasLength<R>::_,
size_t> LengthOf(const R& list) {return list.size();}

template<typename T, size_t N> size_t LengthOf(T(&)[N]) {return N;}

template<typename R> forceinline ReturnTypeOfDataOrDisable<R> DataOf(R&& r) {return r.Data();}

template<typename R> forceinline Meta::EnableIf<
	!HasData<R>::_ && !Has_begin_end<R>::_,
ReturnTypeOf_data_OrDisable<R>> DataOf(R&& r) {return r.data();}

template<typename R, typename=Meta::EnableIf<
	!HasData<R>::_ && Has_data<R>::_ && Has_begin_end<R>::_
>> forceinline decltype(&*begin(Meta::Val<R>())) DataOf(R&& r) {return &*begin(r);}

template<typename T> forceinline const T* DataOf(InitializerList<T> list) {return list.begin();}

template<typename T, size_t N> T* DataOf(T(&arr)[N]) {return arr;}

INTRA_DEFINE_EXPRESSION_CHECKER(HasDataOf, static_cast<const void*>(DataOf(Meta::Val<T>())));
INTRA_DEFINE_EXPRESSION_CHECKER(HasLengthOf, static_cast<size_t>(LengthOf(Meta::Val<T>())));

template<typename R> struct IsArrayClass: Meta::TypeFromValue<bool,
	HasDataOf<R>::_ && HasLengthOf<R>::_
> {};

namespace D {
template<typename R, bool = HasDataOf<R>::_> struct PtrElementTypeOfArray {typedef void _;};
template<typename R> struct PtrElementTypeOfArray<R, true> {typedef decltype(DataOf(Meta::Val<R>())) _;};
template<typename R, bool = HasDataOf<R>::_> struct PtrElementTypeOfArrayOrDisable;
template<typename R> struct PtrElementTypeOfArrayOrDisable<R, true> {typedef decltype(DataOf(Meta::Val<R>())) _;};
}

template<typename T> using PtrElementTypeOfArray = typename D::PtrElementTypeOfArray<T, HasDataOf<T>::_>::_;
template<typename T> using RefElementTypeOfArray = Meta::RemovePointer<PtrElementTypeOfArray<T>>&;
template<typename T> using ElementTypeOfArray = Meta::RemoveConstPointer<PtrElementTypeOfArray<T>>;
template<typename T> using ElementTypeOfArrayKeepConst = Meta::RemovePointer<PtrElementTypeOfArray<T>>;
template<typename T> using PtrElementTypeOfArrayOrDisable = typename D::PtrElementTypeOfArrayOrDisable<T, HasDataOf<T>::_>::_;
template<typename T> using RefElementTypeOfArrayOrDisable = Meta::RemovePointer<PtrElementTypeOfArrayOrDisable<T>>&;
template<typename T> using ElementTypeOfArrayOrDisable = Meta::RemoveConstPointer<PtrElementTypeOfArrayOrDisable<T>>;
template<typename T> using ElementTypeOfArrayKeepConstOrDisable = Meta::RemovePointer<PtrElementTypeOfArrayOrDisable<T>>;

//! ���������, ����� �� ������ Rhs ���� �������� ������� Lhs. ���� ��� �� array class, ���������� false.
template<typename Lhs, typename Rhs> struct AreAssignableArrays: Meta::TypeFromValue<bool,
	Meta::TypeEqualsNotVoid<Meta::RemovePointer<PtrElementTypeOfArray<Lhs>>, Meta::RemovePointer<PtrElementTypeOfArray<Rhs>>>::_ ||
	Meta::TypeEqualsNotVoid<Meta::RemovePointer<PtrElementTypeOfArray<Lhs>>, Meta::RemoveConstPointer<PtrElementTypeOfArray<Rhs>>>::_
> {};

template<typename Lhs, typename T> struct IsAssignableToArrayOf: Meta::TypeFromValue<bool,
	Meta::TypeEquals<T, Meta::RemovePointer<PtrElementTypeOfArray<Lhs>>>::_ ||
	Meta::TypeEquals<Meta::RemoveConst<T>, Meta::RemovePointer<PtrElementTypeOfArray<Lhs>>>::_
> {};

template<typename R> struct IsAssignableArrayClass: Meta::TypeFromValue<bool,
	IsArrayClass<R>::_ &&
	!Meta::IsConst<Meta::RemovePointer<PtrElementTypeOfArray<R>>>::_
>
{};

template<typename R>
struct IsTrivCopyableArray: Meta::TypeFromValue<bool,
	IsArrayClass<R>::_ &&
	Meta::IsTriviallyCopyable<ElementTypeOfArray<R>>::_
> {};

template<typename R1, typename R2>
struct IsTrivCopyCompatibleArrayWith: Meta::TypeFromValue<bool,
	IsTrivCopyableArray<R1>::_ &&
	IsArrayClass<R2>::_ &&
	Meta::TypeEqualsIgnoreCV<ElementTypeOfArray<R1>, ElementTypeOfArray<R2>>::_
> {};

template<typename T1, typename T2> struct IsArrayClassOfExactly: Meta::TypeFromValue<bool,
	IsArrayClass<T1>::_ &&
	Meta::TypeEqualsIgnoreCV<ElementTypeOfArray<T1>, T2>::_
> {};

static_assert(IsArrayClass<const char(&)[5]>::_, "IsArrayClass error.");

}}
