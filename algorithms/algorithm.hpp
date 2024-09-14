#pragma once
#include <algorithm>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

namespace mpd {
	template< class InputIt, class Size, class OutputIt >
	OutputIt move_n(InputIt source, Size count, OutputIt dest) {
		while (count > 0) {
			*dest = *source;
			++source;
			++dest;
			--count;
		}
		return dest;
	}
	template< class InputIt, class Size, class OutputIt >
	OutputIt move_backward_n(InputIt source, Size count, OutputIt dest) {
		while (count > 0) {
			--source;
			--dest;
			--count;
			*dest = *source;
		}
		return dest;
	}

	template <class SrcIterator, class DestIterator>
	std::pair<SrcIterator, DestIterator> copy_s(SrcIterator src_first, SrcIterator src_last, DestIterator dest_first, DestIterator dest_last) {
		while (src_first != src_last && dest_first != dest_last) {
			*dest_first = *src_first;
			++dest_first;
			++src_first;
		}
		return {src_first, dest_first};
	}

	template <class SrcIterator, class DestIterator>
	std::pair<SrcIterator, DestIterator> move_s(SrcIterator src_first, SrcIterator src_last, DestIterator dest_first, DestIterator dest_last) {
		while (src_first != src_last && dest_first != dest_last) {
			*dest_first = std::move(*src_first);
			++dest_first;
			++src_first;
		}
		return {src_first, dest_first};
	}

	template <class SrcIterator, class DestIterator>
	std::pair<SrcIterator, DestIterator> move_backward_s(SrcIterator src_first, SrcIterator src_last, DestIterator dest_first, DestIterator dest_last) {
		while (src_first != src_last && dest_first != dest_last) {
			--dest_last;
			--src_last;
			*dest_last = std::move(*src_last);
		}
		return dest_last;
	}
#if __cplusplus >=  201703L
	using std::size;
#else
	template<class C>
	constexpr auto size(const C& c) noexcept(noexcept(c.size())) -> decltype(c.size())
	{ return c.size(); }

	template<class T, std::size_t N>
	constexpr std::size_t size(const T(&array)[N]) noexcept
	 { return N;}
#endif

#if __cplusplus >=  201703L
	using std::find_end;
#else
	template< class ForwardIt1, class ForwardIt2 >
	ForwardIt1 find_end(ForwardIt1 first, ForwardIt1 last, ForwardIt2 s_first, ForwardIt2 s_last) {
		ForwardIt1 raw_last = last;
		// decrement last until we know s_first to s_last could fit
		std::size_t s_count = std::distance(s_first, s_last);
		std::size_t haystack_count = std::distance(first, last);
		if (s_count > haystack_count) return raw_last;
		last = std::prev(last, s_count);
		// then search backwards for matches
		while (first != last) {
			if (std::equal(s_first, s_last, last)) return last;
			--last;
		}
		return raw_last;
	}
#endif

	using std::find_first_of;

	template< class ForwardIt1, class ForwardIt2 >
	ForwardIt1 find_last_of(ForwardIt1 first, ForwardIt1 last, ForwardIt2 s_first, ForwardIt2 s_last) {
		using R1 = std::reverse_iterator<ForwardIt1>;
		using R2 = std::reverse_iterator<ForwardIt2>;
		R1 result = std::find_first_of(R1(last), R1(first), R2(s_last), R2(s_first));
		if (result == R1(first)) return last;
		return result.base();
	}

	template< class ForwardIt1, class ForwardIt2 >
	ForwardIt1 find_first_not_of(ForwardIt1 first, ForwardIt1 last, ForwardIt2 s_first, ForwardIt2 s_last) {
		using T = decltype(*first);
		return std::find_if_not(first, last, [&](const T& v) {return std::find(s_first, s_last, v) != s_last; });
	}

	template< class ForwardIt1, class ForwardIt2 >
	ForwardIt1 find_last_not_of(ForwardIt1 first, ForwardIt1 last, ForwardIt2 s_first, ForwardIt2 s_last) {
		using R1 = std::reverse_iterator<ForwardIt1>;
		using R2 = std::reverse_iterator<ForwardIt2>;
		R1 result = find_first_not_of(R1(last), R1(first), R2(s_last), R2(s_first));
		if (result == R1(first)) return last;
		return result.base();
	}

	template< class InputIt1, class InputIt2, class Cmp= std::less<typename std::iterator_traits<InputIt1>::value_type> >
	constexpr int compare(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Cmp comp = {}) {
		while (first1 != last1) {
			if (first2 == last2) {
				return 1;
			}
			if (comp(*first1, *first2)) {
				return -1;
			} else if (comp(*first2, *first1)) {
				return 1;
			}
			++first1;
			++first2;
		}
		return (first2 == last2) ? 0 : -1;
	}
}
