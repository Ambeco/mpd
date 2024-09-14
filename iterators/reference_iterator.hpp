#pragma once
#include <cassert>
#include <iterator>
#include <type_traits>

namespace mpd {

	template<class T>
	class reference_iterator {
		static_assert(!std::is_reference_v<T>, "T must not be a reference type");
		const T* ptr;
		std::size_t index;

		bool comparable(const reference_iterator& r) const {
			return ptr == r.ptr || ((ptr == nullptr) != (r.ptr == nullptr));
		}
	public:
		using value_type = const T&;
		using difference_type = std::ptrdiff_t;
		using pointer = const T*;
		using reference = const T&;
		using iterator_category = std::random_access_iterator_tag;

		reference_iterator(const reference_iterator& rhs) noexcept = default;
		reference_iterator(const T& value, std::size_t index = 0) noexcept : ptr(&value), index(index) {}
		reference_iterator(std::nullptr_t value, std::size_t index = 0) noexcept : ptr(nullptr), index(index) {}
		reference_iterator(std::size_t index = 0) noexcept : ptr(nullptr), index(index) {}
		~reference_iterator() noexcept {}
		reference_iterator& operator=(const reference_iterator&) noexcept = default;

		pointer operator->() const noexcept { assert(ptr); return ptr; }
		reference operator*() const noexcept { assert(ptr); return *ptr; }

		reference_iterator& operator++() noexcept { ++index; return *this; }
		reference_iterator operator++(int) noexcept { return {*this, index++}; }
		reference_iterator& operator+=(std::size_t o) noexcept { index += o; return *this; }
		friend reference_iterator operator+(const reference_iterator& it, std::size_t o) noexcept { return {*it, it.index + o}; }
		friend reference_iterator operator+(std::size_t o, const reference_iterator& it) noexcept { return {*it, it.index + o}; }
		reference operator[](std::size_t o) const noexcept { assert(ptr); return *ptr; }
		reference_iterator& operator--() noexcept { --index; return *this; }
		reference_iterator operator--(int) noexcept { return {*this, index--}; }
		reference_iterator& operator-=(std::size_t o) noexcept { index -= o; return *this; }
		friend reference_iterator operator-(const reference_iterator& it, std::size_t o) noexcept { return {*it, it.index - o}; }
		friend difference_type operator-(const reference_iterator& l, const reference_iterator& r) noexcept {
			assert(l.comparable(r)); return static_cast<difference_type>(l.index) - r.index;
		}

		friend bool operator==(const reference_iterator& l, const reference_iterator& r) noexcept { assert(l.comparable(r)); return l.index == r.index; }
		friend bool operator!=(const reference_iterator& l, const reference_iterator& r) noexcept { assert(l.comparable(r)); return l.index != r.index; }
		friend bool operator<(const reference_iterator& l, const reference_iterator& r) noexcept { assert(l.comparable(r)); return l.index < r.index; }
		friend bool operator>(const reference_iterator& l, const reference_iterator& r) noexcept { assert(l.comparable(r)); return l.index > r.index; }
		friend bool operator<=(const reference_iterator& l, const reference_iterator& r) noexcept { assert(l.comparable(r)); return l.index <= r.index; }
		friend bool operator>=(const reference_iterator& l, const reference_iterator& r) noexcept { assert(l.comparable(r)); return l.index >= r.index; }

		friend void swap(reference_iterator& l, reference_iterator& r) noexcept { std::swap(l.ptr, r.ptr); std::swap(l.index, r.index); }
	};
	template<class T>
	reference_iterator<T> ref_iter(const T& ref, std::size_t index = 0) noexcept { return reference_iterator<T>{ref, index}; }
}