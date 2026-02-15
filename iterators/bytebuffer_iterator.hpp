#pragma once
#include <cassert>
#include <iterator>
#include <type_traits>
#include "memory/memory.hpp"
#include "utilities/macros.hpp"

namespace mpd {

	/*
	* Exposes byte buffers as if they were other trivial types instead.
	* 
	* This is incredibly dangerous, but can be handy for things like string hashing, where processing 8+
	* bytes in parallel makes a performance difference.
	* Note that this requires that the BYTE size of the underlying buffer is a multiple of size of exposedT.
	* This does NOT require that the underlying buffer be aligned, but being aligned is probably helpful.
	*/
	template<class underlyingT, class exposedT>
	class bytebuffer_iterator {
		static_assert(!std::is_reference_v<underlyingT>, "underlyingT must not be a reference type");
		static_assert(!std::is_reference_v<exposedT>, "exposedT must not be a reference type");
		static_assert(std::is_trivial<underlyingT>::value, "underlyingT must be trivial");
		static_assert(std::is_trivial<exposedT>::value, "exposedT must be trivial");
		static_assert(!std::is_array_v<exposedT>, "exposedT must not be an array");
		static_assert(sizeof(underlyingT) < sizeof(exposedT), "exposedT must be bigger than underlying");
		static_assert(sizeof(exposedT) > sizeof(underlyingT), "there's no reason to use bytebuffer_iterator for large types");
		static_assert(sizeof(exposedT) % sizeof(underlyingT) == 0, "sizeof(exposedT) must be a multiple of the sizeof(underlyingT)");
		static const unsigned step_size = sizeof(exposedT) / sizeof(underlyingT);

		const underlyingT* ptr;
		mutable exposedT value = {};

		void fill(std::size_t o=0) const {
			assume(mpd::is_aligned_ptr(ptr+0, alignof(exposedT))); //make it obvious to compiler that ptr is already aligned
			std::memcpy(&value, ptr + o, sizeof(exposedT)); 
		}
	public:
		using value_type = exposedT;
		using difference_type = std::ptrdiff_t;
		using pointer = exposedT*;
		using reference = exposedT;
		using iterator_category = std::input_iterator_tag;

		bytebuffer_iterator(const bytebuffer_iterator& rhs) noexcept = default;
		explicit bytebuffer_iterator(const underlyingT* p) noexcept : ptr(p) { assume(p); assume(mpd::is_aligned_ptr(ptr, alignof(exposedT))); }
		~bytebuffer_iterator() noexcept {}
		bytebuffer_iterator& operator=(const bytebuffer_iterator&) noexcept = default;

		pointer operator->() const noexcept { fill(); return &value; }
		reference operator*() const noexcept { fill(); return value; }

		bytebuffer_iterator& operator++() noexcept { ptr += step_size; return *this; }
		bytebuffer_iterator operator++(int) noexcept { auto t = ptr; ptr += step_size; return bytebuffer_iterator{t}; }
		bytebuffer_iterator& operator+=(std::size_t o) noexcept { ptr += o*step_size; return *this; }
		friend bytebuffer_iterator operator+(const bytebuffer_iterator& it, std::size_t o) noexcept { return {*ptr + o*step_size}; }
		friend bytebuffer_iterator operator+(std::size_t o, const bytebuffer_iterator& it) noexcept { return {*ptr + o*step_size}; }
		reference operator[](std::size_t o) const noexcept { fill(o); return *ptr; }
		bytebuffer_iterator& operator--() noexcept { ptr -= step_size; return *this; }
		bytebuffer_iterator operator--(int) noexcept { auto t = ptr; ptr -= step_size; return bytebuffer_iterator{t}; }
		bytebuffer_iterator& operator-=(std::size_t o) noexcept { ptr -= o * step_size; return *this; }
		friend bytebuffer_iterator operator-(const bytebuffer_iterator& it, std::size_t o) noexcept { return {ptr - o* step_size}; }
		friend difference_type operator-(const bytebuffer_iterator& l, const bytebuffer_iterator& r) noexcept { return (l.ptr - r.ptr)/ step_size; }

		friend bool operator==(const bytebuffer_iterator& l, const bytebuffer_iterator& r) noexcept { return l.ptr == r.ptr; }
		friend bool operator!=(const bytebuffer_iterator& l, const bytebuffer_iterator& r) noexcept { return l.ptr != r.ptr; }
		friend bool operator<(const bytebuffer_iterator& l, const bytebuffer_iterator& r) noexcept { return l.ptr < r.ptr; }
		friend bool operator>(const bytebuffer_iterator& l, const bytebuffer_iterator& r) noexcept { return l.ptr > r.ptr; }
		friend bool operator<=(const bytebuffer_iterator& l, const bytebuffer_iterator& r) noexcept { return l.ptr <= r.ptr; }
		friend bool operator>=(const bytebuffer_iterator& l, const bytebuffer_iterator& r) noexcept { return l.ptr >= r.ptr; }

		friend void swap(bytebuffer_iterator& l, bytebuffer_iterator& r) noexcept { std::swap(l.ptr, r.ptr); std::swap(l.index, r.index); }
	};

	template<class underlyingT, std::size_t array_size, std::size_t array_alignment, class exposedT>
	constexpr bool is_useful_bytebuffer_type_v =
		std::is_trivial_v<underlyingT> &&
		sizeof(exposedT) > sizeof(underlyingT) &&
		sizeof(exposedT) % sizeof(underlyingT) == 0 &&
		array_alignment >= alignof(exposedT) &&
		array_alignment % alignof(exposedT) == 0 &&
		sizeof(underlyingT) * array_size % sizeof(__uint128_t) == 0;


	template<class underlyingT, std::size_t array_size, std::size_t array_alignment>
	using best_bytebuffer_type_t =
		std::conditional_t<is_useful_bytebuffer_type_v<underlyingT, array_size, array_alignment, unsigned long long>, unsigned long long,
		std::conditional_t<is_useful_bytebuffer_type_v<underlyingT, array_size, array_alignment, unsigned>, unsigned,
		underlyingT>>;

	template<class underlyingT, class exposedT>
	using bytebuffer_iterator_for = std::conditional_t<std::is_same_v<underlyingT, exposedT>, const underlyingT*, mpd::bytebuffer_iterator<underlyingT, exposedT>>;
}