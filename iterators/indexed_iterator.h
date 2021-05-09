#pragma once
#include <iterator>
#include <type_traits>

namespace mpd {
	template<class base_iterator>
	class indexed_iterator {
		base_iterator base;
		std::size_t index=0;
	public:
		using value_type = std::iterator_traits<base_iterator>::value_type;
		using difference_type = std::iterator_traits<base_iterator>::difference_type;
		using pointer = std::iterator_traits<base_iterator>::pointer;
		using reference = std::iterator_traits<base_iterator>::reference;
		using iterator_category = std::iterator_traits<base_iterator>::iterator_category;

		indexed_iterator() =default;
		indexed_iterator(const indexed_iterator& rhs) =default;
		explicit indexed_iterator(std::size_t index) : index(index) {}
		explicit indexed_iterator(base_iterator base, std::size_t index=0) : base(base), index(index) {}
		~indexed_iterator() {}
		indexed_iterator& operator=(const indexed_iterator&) =default;

		pointer operator->() const {return base.operator->());}
		reference operator*() const {return *base;}

		indexed_iterator& operator++() {++base; ++index; return *this;}
		indexed_iterator operator++(int) {return {base++, index++}; }
		indexed_iterator& operator+=(std::size_t o) {base+=o; index+=o; return *this;}
		friend indexed_iterator operator+(const indexed_iterator& it, std::size_t o) {return {it.base+o, it.index+o};}
		friend indexed_iterator operator+(std::size_t o, const indexed_iterator& it) {return {it.base+o, it.index+o};}
		reference operator[](std::size_t o) const {return base[o];}
		indexed_iterator& operator--() {--base; --index; return *this;}
		indexed_iterator operator--(int) {return {base--, index--}; }
		indexed_iterator& operator-=(std::size_t o) {base-=o; index-=o; return *this;} 
		friend indexed_iterator operator-(const indexed_iterator& it, std::size_t o) {return {it.base-o, it.index-o};}
		friend difference_type operator-(const indexed_iterator& l, const indexed_iterator& r) {return static_cast<difference_type>(l.index) - r.index;}

		friend bool operator==(const indexed_iterator& l, const indexed_iterator& r) {return l==r || l.index==r.index;}
		friend bool operator!=(const indexed_iterator& l, const indexed_iterator& r) {return !(l==r);}
		friend bool operator<(const indexed_iterator& l, const indexed_iterator& r) {return l.index<r.index;}
		friend bool operator>(const indexed_iterator& l, const indexed_iterator& r) {return l.index>r.index;}
		friend bool operator<=(const indexed_iterator& l, const indexed_iterator& r) {return l.index<=r.index;}
		friend bool operator>=(const indexed_iterator& l, const indexed_iterator& r) {return l.index>=r.index;}

		friend void swap(indexed_iterator& l, indexed_iterator& r) {std::swap(l.ptr, r.ptr); std::swap(l.index, r.index);}
	};
}