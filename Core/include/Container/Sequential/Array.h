﻿#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/InitializerList.h"
#include "Platform/CppWarnings.h"
#include "Container/ForwardDecls.h"
#include "Range/Generators/ArrayRange.h"
#include "Algo/Comparison/Equals.h"
#include "Algo/Search/Single.h"
#include "Algo/Mutation/Copy.h"
#include "Platform/InitializerList.h"
#include "Memory/Memory.h"
#include "Range/AsRange.h"
#include "Container/Operations.hh"
#include "Memory/Allocator/Global.h"


namespace Intra { namespace Container {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> class Array
{
public:
	Array(null_t=null): buffer(null), range(null) {}

	explicit Array(size_t initialCapacity):
		buffer(null), range(null) {Reserve(initialCapacity);}

	Array(InitializerList<T> values):
		Array(ArrayRange<const T>(values)) {}
	
	Array(ArrayRange<const T> values): buffer(null), range(null)
	{
		SetCountUninitialized(values.Length());
		Memory::CopyInit(range, values);
	}

	template<size_t N> Array(const T(&values)[N]): Array(Range::AsRange(values)) {}

	template<typename R, typename = Meta::EnableIf<
		!Meta::TypeEqualsIgnoreCVRef<R, Array>::_ &&
		Range::IsAsConsumableRangeOf<R, T>::_ &&
		(Range::IsAsForwardRange<R>::_ || Range::HasLength<Range::AsRangeResult<R>>::_)
	>> Array(R&& values): buffer(null), range(null)
	{AddLastRange(Range::Forward<R>(values));}

	Array(Array&& rhs):
		buffer(rhs.buffer), range(rhs.range)
	{rhs.buffer = null; rhs.range = null;}

	Array(const Array& rhs): Array(rhs.AsConstRange()) {}
	
	~Array() {operator=(null);}


	//! Создать контейнер из уже выделенного диапазона.
	//! Внимание: rangeToOwn должен быть выделен тем же аллокатором, что и шаблонный аргумент Allocator!
	static Array CreateAsOwnerOf(ArrayRange<T> rangeToOwn)
	{
		Array result;
		result.range = result.buffer = rangeToOwn;
		return result;
	}

	static Array CreateWithCount(size_t count)
	{
		Array result;
		result.SetCount(count);
		return result;
	}

	static Array CreateWithCount(size_t count, const T& initValue)
	{
		Array result;
		result.SetCount(count, initValue);
		return result;
	}


	Array& operator=(const Array& rhs)
	{
		if(this==&rhs) return *this;
		Assign(rhs.AsConstRange());
		return *this;
	}

	Array& operator=(Array&& rhs)
	{
		Clear();
		range = rhs.range;
		rhs.range.End = rhs.range.Begin;
		Meta::Swap(buffer, rhs.buffer);
		return *this;
	}

	Array& operator=(ArrayRange<const T> values) {return operator=(Array(values));}
	template<typename R> Meta::EnableIf<
		Range::IsAsAccessibleRange<R>::_,
	Array&> operator=(R&& values) {return operator=(Array(values));}

	//! Удалить все элементы и освободить память.
	Array& operator=(null_t)
	{
		Clear();
		Memory::FreeRangeUninitialized(Memory::GlobalHeap, buffer);
		buffer = null;
		range = null;
		return *this;
	}

	template<typename U> void Assign(ArrayRange<const U> rhs)
	{
		Clear();
		SetCountUninitialized(rhs.Length());
		Memory::CopyInit(range, rhs);
	}

	template<typename U> void Assign(ArrayRange<U> rhs) {Assign(rhs.AsConstRange());}

	//! Добавить новый элемент в начало массива копированием или перемещением value.
	forceinline T& AddFirst(T&& value)
	{
		INTRA_DEBUG_ASSERT(!range.ContainsAddress(Meta::AddressOf(value)));
		if(NoLeftSpace()) CheckSpace(0, 1);
		return *new(--range.Begin) T(Meta::Move(value));
	}

	forceinline T& AddFirst(const T& value)
	{
		INTRA_DEBUG_ASSERT(!range.ContainsAddress(Meta::AddressOf(value)));
		if(NoLeftSpace()) CheckSpace(0, 1);
		return *new(--range.Begin) T(value);
	}

	//! Добавить новый элемент в начало массива, сконструировав его на месте с параметрами args.
	template<typename... Args> forceinline Meta::EnableIf<
		Meta::IsConstructible<T, Args...>::_,
	T&> EmplaceFirst(Args&&... args)
	{
		if(NoLeftSpace()) CheckSpace(0, 1);
		return *new(--range.Begin) T(Meta::Forward<Args>(args)...);
	}
	
	//! Добавить все значения указанного диапазона в начало массива
	template<typename R> Meta::EnableIf<
		Range::IsAsConsumableRangeOf<R, T>::_ &&
		(Range::IsForwardRange<R>::_ || Range::HasLength<R>::_)
	> AddFirstRange(R&& values)
	{
		auto valueRange = Range::Forward<R>(values);
		//INTRA_DEBUG_ASSERT(!range.Overlaps((ArrayRange<const byte>&)values));
		const size_t valuesCount = Range::Count(values);
		if(LeftSpace()<valuesCount) CheckSpace(0, valuesCount);
		for(T* dst = (range.Begin -= valuesCount); !valueRange.Empty(); valueRange.PopFirst())
			new(dst++) T(valueRange.First());
	}

	//! Добавить все значения указанного диапазона в начало массива
	template<typename R> Meta::EnableIf<
		Range::IsAsConsumableRangeOf<R, T>::_ &&
		!(Range::IsForwardRange<R>::_ || Range::HasLength<R>::_)
	> AddFirstRange(R&& values)
	{
		Array temp = Range::Forward<R>(values);
		CheckSpace(0, temp.Length());
		Memory::MoveInit({range.Begin-temp.Length(), range.Begin}, temp.AsRange());
		range.Begin -= temp.Length();
	}

	//! Добавить новый элемент в конец массива перемещением value.
	forceinline T& AddLast(T&& value)
	{
		INTRA_DEBUG_ASSERT(!range.ContainsAddress(Meta::AddressOf(value)));
		if(NoRightSpace()) CheckSpace(1, 0);
		return *new(range.End++) T(Meta::Move(value));
	}

	//! Добавить новый элемент в конец массива копированием value.
	forceinline T& AddLast(const T& value)
	{
		INTRA_DEBUG_ASSERT(!range.ContainsAddress(Meta::AddressOf(value)));
		if(NoRightSpace()) CheckSpace(1, 0);
		return *new(range.End++) T(value);
	}

	//! Добавить новый элемент в конец массива, сконструировав его на месте с параметрами args.
	template<typename... Args> forceinline Meta::EnableIf<
		Meta::IsConstructible<T, Args...>::_,
	T&> EmplaceLast(Args&&... args)
	{
		if(NoRightSpace()) CheckSpace(1, 0);
		return *new(range.End++) T(Meta::Forward<Args>(args)...);
	}

	//! Добавить все значения указанного диапазона в конец массива.
	template<typename R> Meta::EnableIf<
		Range::IsAsConsumableRangeOf<R, T>::_ &&
		(Range::IsForwardRange<R>::_ || Range::HasLength<R>::_)
	> AddLastRange(R&& values)
	{
		auto valueRange = Range::Forward<R>(values);
		//INTRA_DEBUG_ASSERT(!data.Overlaps((ArrayRange<const byte>&)values));
		const size_t valuesCount = Range::Count(valueRange);
		if(RightSpace()<valuesCount) CheckSpace(valuesCount, 0);
		for(; !valueRange.Empty(); valueRange.PopFirst())
			new(range.End++) T(valueRange.First());
	}

	//! Добавить все значения указанного диапазона в конец массива.
	template<typename R> Meta::EnableIf<
		Range::IsAsConsumableRangeOf<R, T>::_ &&
		!(Range::IsForwardRange<R>::_ || Range::HasLength<R>::_)
	> AddLastRange(R&& values)
	{
		auto valueRange = Range::Forward<R>(values);
		for(; !valueRange.Empty(); valueRange.PopFirst())
		{
			if(RightSpace()<1) CheckSpace(1, 0);
			new(range.End++) T(valueRange.First());
		}
	}

	//! Установить элемент с индексом pos в value. Если pos>=Count(), в массив будет добавлено pos-Count()
	//! элементов конструктором по умолчанию и один элемент конструктором копирования или перемещения от value.
	template<typename U> T& Set(size_t pos, U&& value)
	{
		if(pos>=Count())
		{
			Reserve(pos+1);
			SetCount(pos);
			return AddLast(Meta::Forward<U>(value));
		}
		operator[](pos) = Meta::Forward<U>(value);
	}
	

	//! Вставить новые элементы в указанную позицию.
	//! Если pos<(Count()+values.Count())/2 и LeftSpace()>=values.Count(), элементы с индексами < pos
	//! будут перемещены конструктором перемещения и деструктором на values.Count() элементов назад.
	//! Если pos>=(Count()+values.Count())/2 или LeftSpace()<values.Count(), элементы с индексами >= pos
	//! будут перемещены конструктором перемещения и деструктором на values.Count() элементов вперёд.
	//! Элементы, имеющие индексы >= pos, будут иметь индексы, увеличенные на values.Count().
	//! Первый вставленный элемент будет иметь индекс pos.
	template<typename U> void Insert(size_t pos, ArrayRange<const U> values)
	{
		if(values.Empty()) return;
		INTRA_DEBUG_ASSERT(!range.Overlaps(values));
		const size_t valuesCount = values.Count();

		//Если не хватает места, перераспределяем память и копируем элементы
		if(Count()+valuesCount>Capacity())
		{
			size_t newCapacity = Count()+valuesCount+Capacity()/2;
			ArrayRange<T> newBuffer = Memory::AllocateRangeUninitialized<T>(
				Memory::GlobalHeap, newCapacity, INTRA_SOURCE_INFO);
			ArrayRange<T> newRange = newBuffer.Drop(LeftSpace()).Take(Count()+valuesCount);
			Memory::MoveInitDelete(newRange.Take(pos), range.Take(pos));
			Memory::MoveInitDelete(newRange.Drop(pos+valuesCount), range.Drop(pos));
			Memory::CopyInit(newRange.Drop(pos).Take(valuesCount), values);
			Memory::FreeRangeUninitialized(Memory::GlobalHeap, buffer);
			buffer = newBuffer;
			range = newRange;
			return;
		}

		//Добавляем элемент, перемещая ближайшую к концу часть массива
		if(pos>=(Count()+valuesCount)/2 || LeftSpace()<valuesCount)
		{
			range.End += valuesCount;
			Memory::MoveInitDeleteBackwards<T>(range.Drop(pos+valuesCount), range.Drop(pos).DropBack(valuesCount));
		}
		else
		{
			range.Begin -= valuesCount;
			Memory::MoveInitDelete<T>(range.Take(pos), range.Drop(valuesCount).Take(pos));
		}
		Memory::CopyInit<T>(range.Drop(pos).Take(valuesCount), values);
	}

	template<typename U> forceinline void Insert(const T* it, ArrayRange<const U> values)
	{
		INTRA_DEBUG_ASSERT(range.ContainsAddress(it));
		Insert(it-range.Begin, values);
	}

	forceinline void Insert(size_t pos, const T& value) {Insert(pos, {&value, 1});}

	forceinline void Insert(const T* it, const T& value)
	{
		INTRA_DEBUG_ASSERT(range.ContainsAddress(it));
		Insert(it-range.Begin, value);
	}


	//! Добавить новый элемент в конец.
	forceinline Array& operator<<(const T& value) {AddLast(value); return *this;}

	//! Прочитать и удалить последний элемент.
	forceinline Array& operator>>(T& value)
	{
		value = Meta::Move(Last());
		RemoveLast();
		return *this;
	}

	//! Возвращает последний элемент, удаляя его из массива.
	forceinline T PopLastElement()
	{
		T result = Meta::Move(Last());
		RemoveLast();
		return result;
	}


	//! Возвращает первый элемент, удаляя его из массива.
	forceinline T PopFirstElement()
	{
		T result = Meta::Move(First());
		RemoveFirst();
		return result;
	}

	//! Установить новый размер буфера массива (не влезающие элементы удаляются).
	void Resize(size_t rightPartSize, size_t leftPartSize=0)
	{
		if(rightPartSize+leftPartSize==0) {*this=null; return;}

		//Удаляем элементы, выходящие за границы массива
		if(rightPartSize <= Count()) Memory::Destruct(range.Drop(rightPartSize));

		size_t newCapacity = rightPartSize+leftPartSize;
		ArrayRange<T> newBuffer = Memory::AllocateRangeUninitialized<T>(
			Memory::GlobalHeap, newCapacity, INTRA_SOURCE_INFO);
		ArrayRange<T> newRange = newBuffer.Drop(leftPartSize).Take(Count());

		if(!buffer.Empty())
		{
			//Перемещаем элементы в новый участок памяти
			Memory::MoveInitDelete<T>(newRange, range);
			Memory::FreeRangeUninitialized(Memory::GlobalHeap, buffer);
		}
		buffer = newBuffer;
		range = newRange;
	}

	//! Убедиться, что буфер массива может вместить rightPart элементов при добавлении
	//! их в конец и имеет leftSpace места для добавления в начало.
	void Reserve(size_t rightPart, size_t leftSpace=0)
	{
		const size_t currentRightPartSize = size_t(buffer.End-range.Begin);
		const size_t currentLeftSpace = LeftSpace();
		if(rightPart<=currentRightPartSize && leftSpace<=currentLeftSpace) return;

		const size_t currentSize = Capacity();
		if(rightPart>0)
			if(leftSpace>0) Resize(currentSize/4+rightPart, currentSize/4+leftSpace);
			else Resize(currentSize/2+rightPart, currentLeftSpace);
		else Resize(currentRightPartSize, currentLeftSpace+currentSize/2+leftSpace);
	}

	//! Убедиться, что буфер массива может вместить rightSpace новых элементов при добавлении их в конец и имеет leftSpace места для добавления в начало.
	forceinline void CheckSpace(size_t rightSpace, size_t leftSpace=0) {Reserve(Count()+rightSpace, leftSpace);}

	//! Удалить все элементы из массива, не освобождая занятую ими память.
	forceinline void Clear()
	{
		if(Empty()) return;
		Memory::Destruct<T>(range);
		range.End = range.Begin;
	}

	//! Возвращает, является ли массив пустым.
	forceinline bool Empty() const {return range.Empty();}

	//! Количество элементов, которые можно вставить в начало массива до перераспределения буфера.
	forceinline size_t LeftSpace() const {return size_t(range.Begin-buffer.Begin);}

	//! Возвращает true, если массив не имеет свободного места для вставки элемента в начало массива.
	forceinline bool NoLeftSpace() const {return range.Begin==buffer.Begin;}

	//! Количество элементов, которые можно вставить в конец массива до перераспределения буфера.
	forceinline size_t RightSpace() const {return size_t(buffer.End-range.End);}

	//! Возвращает true, если массив не имеет свободного места для вставки элемента в конец массива.
	forceinline bool NoRightSpace() const {return buffer.End==range.End;}

	//!@{
	//! Удаление элементов с сохранением порядка. Индексы всех элементов, находящихся после удаляемых элементов, уменьшаются на количество удаляемых элементов.
	//! При наличии итераторов на какие-либо элементы массива следует учесть, что:
	//! при удалении элементов с индексами < Count()/4 все предшествующие элементы перемещаются вправо конструктором перемещения и деструктором.
	//! при удалении элементов с индексами >= Count()/4 все последующие элементы перемещаются влево конструктором перемещения и деструктором.
	//! Таким образом адреса элементов изменяются и указатели станут указывать либо на другой элемент массива, либо на неинициализированную область массива.

	//! Удалить один элемент по индексу.
	void Remove(size_t index)
	{
		INTRA_DEBUG_ASSERT(index<Count());
		range[index].~T();

		// Соотношение 1/4 вместо 1/2 было выбрано, потому что перемещение перекрывающихся
		// участков памяти вправо в ~2 раза медленнее, чем влево
		if(index>=Count()/4) //Перемещаем правую часть влево
		{
			Memory::MoveInitDelete<T>({range.Begin+index, range.End-1}, {range.Begin+index+1, range.End});
			--range.End;
		}
		else //Перемещаем левую часть вправо
		{
			Memory::MoveInitDeleteBackwards<T>({range.Begin+1, range.Begin+index+1}, {range.Begin, range.Begin+index});
			++range.Begin;
		}
	}

	//! Удалить один элемент по указателю.
	forceinline void Remove(T* ptr)
	{
		INTRA_DEBUG_ASSERT(range.ContainsAddress(ptr));
		Remove(ptr-range.Begin);
	}

	//! Удаление всех элементов в диапазоне [removeStart; removeEnd)
	void Remove(size_t removeStart, size_t removeEnd)
	{
		INTRA_DEBUG_ASSERT(removeStart <= removeEnd);
		INTRA_DEBUG_ASSERT(removeEnd <= Count());
		if(removeEnd==removeStart) return;
		const size_t elementsToRemove = removeEnd-removeStart;
		Memory::Destruct<T>(range(removeStart, removeEnd));

		//Быстрые частные случаи
		if(removeEnd==Count())
		{
			range.End -= elementsToRemove;
			return;
		}

		if(removeStart==0)
		{
			range.Begin += elementsToRemove;
			return;
		}

		if(removeStart+elementsToRemove/2>=Count()/4)
		{
			Memory::MoveInitDelete<T>(
				range.Drop(removeStart).DropBack(elementsToRemove),
				range.Drop(removeEnd));
			range.End -= elementsToRemove;
		}
		else
		{
			Memory::MoveInitDeleteBackwards<T>(
				range(elementsToRemove, removeEnd),
				range.Take(removeStart));
			range.Begin += elementsToRemove;
		}
	}

	//! Удаление первого найденного элемента, равного value
	forceinline void FindAndRemove(const T& value)
	{
		size_t found = range.CountUntil(value);
		if(found!=Count()) Remove(found);
	}
	//!@}

	//! Удалить дублирующиеся элементы из массива. При этом порядок элементов в массиве не сохраняется.
	void RemoveDuplicatesUnordered()
	{
		for(size_t i=0; i<Count(); i++)
			for(size_t j=i+1; j<Count(); j++)
				if(operator[](i)==operator[](j))
					RemoveUnordered(j--);
	}

	//! Удалить первый элемент.
	forceinline void RemoveFirst() {INTRA_DEBUG_ASSERT(!Empty()); (range.Begin++)->~T();}

	//! Удалить последний элемент.
	forceinline void RemoveLast() {INTRA_DEBUG_ASSERT(!Empty()); (--range.End)->~T();}

	//!@{
	//! Быстрое удаление путём переноса последнего элемента (без смещения).
	forceinline void RemoveUnordered(size_t index)
	{
		INTRA_DEBUG_ASSERT(index<Count());
		if(index<Count()-1) range[index] = Meta::Move(*--range.End);
		RemoveLast();
	}

	void FindAndRemoveUnordered(const T& value)
	{
		size_t index = Algo::CountUntil(range, value);
		if(index!=Count()) RemoveUnordered(index);
	}
	//!@}

	//! Освободить незанятую память массива, уменьшив буфер до количества элементов в массиве,
	//! если ёмкость превышает количество элементов более, чем на 25%.
	forceinline void TrimExcessCapacity() {if(Capacity()>Count()*5/4) Resize(Count());}




	forceinline T& operator[](size_t index) {INTRA_DEBUG_ASSERT(index<Count()); return range.Begin[index];}
	forceinline const T& operator[](size_t index) const {INTRA_DEBUG_ASSERT(index<Count()); return range.Begin[index];}

	forceinline T& Last() {INTRA_DEBUG_ASSERT(!Empty()); return range.Last();}
	forceinline const T& Last() const {INTRA_DEBUG_ASSERT(!Empty()); return range.Last();}
	forceinline T& First() {INTRA_DEBUG_ASSERT(!Empty()); return range.First();}
	forceinline const T& First() const {INTRA_DEBUG_ASSERT(!Empty()); return range.First();}

	forceinline T* Data() {return begin();}
	forceinline const T* Data() const {return begin();}

	forceinline T* End() {return end();}
	forceinline const T* End() const {return end();}

	//! Возвращает байтовый буфер с данными этого массива. При этом сам массив становится пустым. Деструкторы не вызываются!
	//ByteBuffer MoveToByteBuffer() {return Meta::Move(data);}

	//template<typename U=T> Meta::EnableIfPod<U, ByteBuffer> CopyToByteBuffer() {return data;}
	template<typename U> forceinline Array<U> MoveReinterpret() {return Meta::Move(reinterpret_cast<Array<U>&>(*this));}


	//! Возвращает суммарный размер в байтах элементов в массиве
	forceinline size_t SizeInBytes() const {return Count()*sizeof(T);}

	//!@{
	//! Возвращает количество элементов в массиве
	forceinline size_t Count() const {return range.Length();}
	forceinline size_t Length() const {return Count();}
	//!@}

	//! Изменить количество занятых элементов массива (с удалением лишних элементов или инициализацией по умолчанию новых)
	void SetCount(size_t newCount)
	{
		const size_t oldCount = Count();
		if(newCount<=oldCount)
		{
			Memory::Destruct<T>(range.Drop(newCount));
			range.End = range.Begin+newCount;
			return;
		}
		SetCountUninitialized(newCount);
		Memory::Initialize<T>(range.Drop(oldCount));
	}

	//! Изменить количество занятых элементов массива (с удалением лишних элементов или инициализацией копированием новых)
	void SetCount(size_t newCount, const T& initValue)
	{
		const size_t oldCount = Count();
		if(newCount<=oldCount)
		{
			Memory::Destruct<T>(range.Drop(newCount));
			range.End = range.Begin+newCount;
			return;
		}
		SetCountUninitialized(newCount);
		for(T* dst = range.Begin+oldCount; dst<range.End; dst++)
			new(dst) T(initValue);
	}

	//! Изменить количество занятых элементов массива без вызова лишних элементов или инициализации новых.
	//! Инициализировать\удалять элементы придётся вручную, либо использовать этот метод только для POD типов!
	void SetCountUninitialized(size_t newCount)
	{
		Reserve(newCount, 0);
		if(newCount==0) range.Begin = buffer.Begin;
		range.End = range.Begin + newCount;
	}

	//! Получить текущий размер буфера массива
	forceinline size_t Capacity() const {return buffer.Length();}

	forceinline bool IsFull() const {return Count()==Capacity();}


	forceinline operator ArrayRange<T>() {return AsRange();}
	forceinline operator ArrayRange<const T>() const {return AsRange();}
	forceinline ArrayRange<T> AsRange() {return range;}
	forceinline ArrayRange<const T> AsConstRange() const {return range.AsConstRange();}
	forceinline ArrayRange<const T> AsRange() const {return AsConstRange();}

	forceinline ArrayRange<T> operator()(size_t firstIndex, size_t endIndex)
	{
		INTRA_DEBUG_ASSERT(firstIndex <= endIndex);
		INTRA_DEBUG_ASSERT(endIndex <= Count());
		return range(firstIndex, endIndex);
	}

	forceinline ArrayRange<const T> operator()(size_t firstIndex, size_t endIndex) const
	{
		INTRA_DEBUG_ASSERT(firstIndex <= endIndex);
		INTRA_DEBUG_ASSERT(endIndex <= Count());
		return AsConstRange()(firstIndex, endIndex);
	}

	ArrayRange<T> Take(size_t count) {return range.Take(count);}
	ArrayRange<const T> Take(size_t count) const {return AsConstRange().Take(count);}
	ArrayRange<T> Drop(size_t count) {return range.Drop(count);}
	ArrayRange<const T> Drop(size_t count) const {return AsConstRange().Drop(count);}


	//! @defgroup Array_STL_Interface STL-подобный интерфейс для Array
	//! Этот интерфейс предназначен для совместимости с обобщённым контейнеро-независимым кодом.
	//! Использовать напрямую этот интерфейс не рекомендуется.
	//!@{
	typedef T value_type;
	typedef T* iterator;
	typedef const T* const_iterator;
	forceinline void push_back(T&& value) {AddLast(Meta::Move(value));}
	forceinline void push_back(const T& value) {AddLast(value);}
	forceinline void push_front(T&& value) {AddFirst(Meta::Move(value));}
	forceinline void push_front(const T& value) {AddFirst(value);}
	forceinline void pop_back() {RemoveLast();}
	forceinline void pop_front() {RemoveFirst();}
	forceinline T& back() {return Last();}
	forceinline const T& back() const {return Last();}
	forceinline T& front() {return First();}
	forceinline const T& front() const {return First();}
	forceinline bool empty() const {return Empty();}
	forceinline T* data() {return Data();}
	forceinline const T* data() const {return Data();}
	forceinline T& at(size_t index) {return operator[](index);}
	forceinline const T& at(size_t index) const {return operator[](index);}
	forceinline T* begin() {return range.Begin;}
	forceinline const T* begin() const {return range.Begin;}
	forceinline T* end() {return range.End;}
	forceinline const T* end() const {return range.End;}
	forceinline const T* cbegin() const {return begin();}
	forceinline const T* cend() const {return end();}
	forceinline size_t size() const {return Count();}
	forceinline size_t capacity() const {return Capacity();}
	forceinline void shrink_to_fit() {TrimExcessCapacity();}
	forceinline void clear() {Clear();}

	forceinline iterator insert(const_iterator pos, const T& value) {Insert(size_t(pos-Data()), value);}
	forceinline iterator insert(const_iterator pos, T&& value) {Insert(size_t(pos-Data()), Meta::Move(value));}

	forceinline iterator insert(const_iterator pos, size_t count, const T& value);
	template<typename InputIt> forceinline iterator insert(const_iterator pos, InputIt first, InputIt last);

	forceinline iterator insert(const T* pos, std::initializer_list<T> ilist)
	{Insert(size_t(pos-Data()), ArrayRange<const T>(ilist));}

	//! Отличается от std::vector<T>::erase тем, что инвалидирует все итераторы,
	//! а не только те, которые идут после удаляемого элемента.
	forceinline T* erase(const_iterator pos)
	{
		Remove(size_t(pos-Data()));
		return Data()+(pos-Data());
	}

	forceinline T* erase(const_iterator firstPtr, const_iterator endPtr)
	{
		Remove(size_t(firstPtr-Data()), size_t(endPtr-Data()));
		return Data()+(firstPtr-Data());
	}

	forceinline void reserve(size_t newCapacity) {Reserve(newCapacity);}
	forceinline void resize(size_t newCount) {SetCount(newCount);}
	//void resize(size_t count, const T& value);

	forceinline void swap(Array& rhs)
	{
		Meta::Swap(range, rhs.range);
		Meta::Swap(buffer, rhs.buffer);
	}
	//!@}


private:
	ArrayRange<T> buffer, range;
};

template<typename T> forceinline T* begin(Array<T>& arr) {return arr.begin();}
template<typename T> forceinline const T* begin(const Array<T>& arr) {return arr.begin();}
template<typename T> forceinline T* end(Array<T>& arr) {return arr.end();}
template<typename T> forceinline const T* end(const Array<T>& arr) {return arr.end();}


}

namespace Meta {
template<typename T> struct IsTriviallyRelocatable<Array<T>>: TypeFromValue<bool, true> {};
}

}

INTRA_WARNING_POP
