#pragma once
#include <iterator>
#include <memory>
#include <utility>

namespace mpd {

#if __cplusplus >=  201703L
	using std::destroy;
	using std::destroy_at;
#else
	template< class T >
	constexpr void destroy_at(T* p) {
		p->~T();
	}
	template< class ForwardIt >
	constexpr void destroy(ForwardIt first, ForwardIt last) {
		using T = typename  std::iterator_traits<ForwardIt>::value_type;
		while (first != last)
			std::addressof(*--last)->~T();
	}
#endif

#if _cplusplus >= 202303L
	using std::construct_at
#else
	template<class T, class...Args>
	constexpr T* construct_at(T* p, Args&&...args) {
		return ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
	}
#endif
	template<class T>
	constexpr T* value_construct_at(T* p) {
		return ::new (static_cast<void*>(&*p)) T;
	}
	template<class Iterator, class T = typename std::iterator_traits<Iterator>::value_type, class...Args>
	constexpr T* indirect_construct_at(Iterator p, Args&&...args) {
		return ::new (static_cast<void*>(&*p)) T(std::forward<Args>(args)...);
	}
	template<class Iterator, class T = typename std::iterator_traits<Iterator>::value_type, class...Args>
	constexpr T* indirect_value_construct_at(Iterator p) {
		return ::new (static_cast<void*>(&*p)) T;
	}

#if _cplusplus >= 201703L
	using std::uninitialized_value_construct;
	using std::uninitialized_default_construct;
#else
	template< class ForwardIt >
	void uninitialized_value_construct(ForwardIt first, ForwardIt last) {
		ForwardIt current = first;
		try {
			for (; current != last; ++current)
				indirect_construct_at(current);
		} catch (...) {
			destroy(first, current);
			throw;
		}
	}

	template< class ForwardIt >
	void uninitialized_default_construct(ForwardIt first, ForwardIt last) {
		using Value = typename std::iterator_traits<ForwardIt>::value_type;
		ForwardIt current = first;
		try {
			for (; current != last; ++current)
				indirect_value_construct_at(current);
		} catch (...) {
			destroy(first, current);
			throw;
		}
	}
#endif

#if _cplusplus >= 201703L
	using std::uninitialized_move;
	using std::uninitialized_move_n;
#else
	template< class InputIt, class NoThrowForwardIt >
	NoThrowForwardIt uninitialized_move(InputIt src_first, InputIt src_last, NoThrowForwardIt d_first) {
		using Value = typename std::iterator_traits<NoThrowForwardIt>::value_type;
		NoThrowForwardIt current = d_first;
		try {
			for (; src_first != src_last; ++src_first, (void) ++current)
				indirect_construct_at(current, std::move(*src_first));
			return current;
		} catch (...) {
			destroy(d_first, current);
			throw;
		}
	}

	template< class InputIt, class Size, class NoThrowForwardIt >
	std::pair<InputIt, NoThrowForwardIt> uninitialized_move_n(InputIt src_first, Size count, NoThrowForwardIt d_first) {
		NoThrowForwardIt current = d_first;
		try {
			for (; count > 0; ++src_first, (void) ++current, --count)
				indirect_construct_at(current, std::move(*src_first));
		} catch (...) {
			destroy(d_first, current);
			throw;
		}
		return {src_first, current};

	}
#endif

	template<class SourceIt, class DestIt>
	std::pair<SourceIt, DestIt> uninitialized_copy_s(SourceIt src_first, SourceIt src_last, DestIt dest_first, DestIt dest_last) {
		DestIt first_dest = dest_first;
		try {
			while (src_first != src_last && dest_first != dest_last) {
				indirect_construct_at(dest_first, *src_first);
				++src_first;
				++dest_first;
			}
			return {src_first, dest_first};
		} catch (...) {
			destroy(first_dest, dest_first);
			throw;
		}
	}

	template<class SourceIt, class DestIt>
	std::pair<SourceIt, DestIt> uninitialized_move_s(SourceIt src_first, SourceIt src_last, DestIt dest_first, DestIt dest_last) {
		auto pair = uninitialized_copy(std::make_move_iterator(src_first), std::make_move_iterator(src_last), dest_first, dest_last);
		return {pair.first.base(), pair.second};
	}
}