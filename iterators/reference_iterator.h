#pragma once
#include <iterator>
#include <type_traits>

namespace mpd {
	template<class T>
	class reference_iterator {
		static_assert(!std::is_reference_t<T>, "T must not be a reference type");
		const T* ptr;
		std::size_t index=0;
	public:
		using value_type = const T&;
		using difference_type = std::ptrdiff_t;
		using pointer = const T*;
		using reference = const T&;
		using iterator_category = std::random_access_iterator_tag;

		reference_iterator() : ptr(nullptr_t) {}
		reference_iterator(const reference_iterator& rhs) =default;
		explicit reference_iterator(const T& value, std::size_t index=0) : ptr(&value), index(index) {}
		~reference_iterator() {}
		reference_iterator& operator=(const reference_iterator&) =default;

		pointer operator->() const {return ptr;}
		reference operator*() const {return *ptr;}

		reference_iterator& operator++() {++index; return *this;}
		reference_iterator operator++(int) {return {*this, index++}; }
		reference_iterator& operator+=(std::size_t o) {index+=o; return *this;}
		friend reference_iterator operator+(const reference_iterator& it, std::size_t o) {return {*it, it.index+o};}
		friend reference_iterator operator+(std::size_t o, const reference_iterator& it) {return {*it, it.index+o};}
		reference operator[](std::size_t o) const {return *this;}
		reference_iterator& operator--() {--index; return *this;}
		reference_iterator operator--(int) {return {*this, index--}; }
		reference_iterator& operator-=(std::size_t o) {index-=o; return *this;} 
		friend reference_iterator operator-(const reference_iterator& it, std::size_t o) {return {*it, it.index-o};}
		friend difference_type operator-(const reference_iterator& l, const reference_iterator& r) {return static_cast<difference_type>(l.index) - r.index;}

		friend bool operator==(const reference_iterator& l, const reference_iterator& r) {assert(l.ptr==r.ptr); return l.index==r.index;}
		friend bool operator!=(const reference_iterator& l, const reference_iterator& r) {assert(l.ptr==r.ptr); return l.index!=r.index;}
		friend bool operator<(const reference_iterator& l, const reference_iterator& r) {assert(l.ptr==r.ptr); return l.index<r.index;}
		friend bool operator>(const reference_iterator& l, const reference_iterator& r) {assert(l.ptr==r.ptr); return l.index>r.index;}
		friend bool operator<=(const reference_iterator& l, const reference_iterator& r) {assert(l.ptr==r.ptr); return l.index<=r.index;}
		friend bool operator>=(const reference_iterator& l, const reference_iterator& r) {assert(l.ptr==r.ptr); return l.index>=r.index;}

		friend void swap(reference_iterator& l, reference_iterator& r) {std::swap(l.ptr, r.ptr); std::swap(l.index, r.index);}
	};
}