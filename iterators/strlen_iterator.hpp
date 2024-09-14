#pragma once
#include <iterator>

namespace mpd {
	template<class T>
	class strlen_iterator {
		const T* ptr;
	public:
		using value_type = const T&;
		using difference_type = std::ptrdiff_t;
		using pointer = const T*;
		using reference = const T&;
		using iterator_category = std::bidirectional_iterator_tag;

		strlen_iterator() noexcept : ptr(nullptr) {}
		strlen_iterator(const strlen_iterator& rhs) noexcept = default;
		strlen_iterator(const T* value) noexcept : ptr(value) {}
		~strlen_iterator() noexcept {}
		strlen_iterator& operator=(const strlen_iterator&) noexcept = default;

		pointer operator->() const noexcept { assert(ptr); return ptr; }
		reference operator*() const noexcept { assert(ptr); return *ptr; }

		strlen_iterator& operator++() noexcept { ++ptr; return *this; }
		strlen_iterator operator++(int) noexcept { return {*this, ptr++}; }
		strlen_iterator& operator--() noexcept { --ptr; return *this; }
		strlen_iterator operator--(int) noexcept { return {*this, ptr--}; }

		friend bool operator==(const strlen_iterator& l, const strlen_iterator& r) noexcept {
			if (l.ptr == nullptr) {
				if (r.ptr == nullptr) return true;
				return (r.ptr[0] == '\0');
			} else {
				if (r.ptr == nullptr) return (l.ptr[0] == '\0');
				return l.ptr == r.ptr;
			}
		}
		friend bool operator!=(const strlen_iterator& l, const strlen_iterator& r) noexcept { return !operator==(l, r); }

		friend void swap(strlen_iterator& l, strlen_iterator& r) noexcept { std::swap(l.ptr, r.ptr); std::swap(l.index, r.index); }
	};
	template<class T>
	strlen_iterator<T> strlen_iter(const T* ptr) noexcept { return strlen_iterator<T>{ptr}; }
}