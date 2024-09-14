#pragma once
#include <algorithm>
#include <iterator>

namespace mpd {
	namespace impl {
		template< class ForwardIt>
		constexpr typename std::iterator_traits<ForwardIt>::difference_type 
			distance_up_to_n(ForwardIt first, ForwardIt last, typename std::iterator_traits<ForwardIt>::difference_type n, std::forward_iterator_tag)
			noexcept(noexcept(first != last)
				&& std::is_nothrow_constructible_v<ForwardIt>
				&& noexcept(++first)) {
			typename std::iterator_traits<ForwardIt>::difference_type r = 0;
			while (first != last && r < n) {
				++first;
				++r;
			}
			return r;
		}
		template< class ForwardIt>
		constexpr typename std::iterator_traits<ForwardIt>::difference_type
			distance_up_to_n(ForwardIt first, ForwardIt last, typename std::iterator_traits<ForwardIt>::difference_type n, std::random_access_iterator_tag)
			noexcept(noexcept(last - first < n)) {
			return ((last - first) < n) ? (last - first) : n;
		}
	}
	using std::distance;
	template< class ForwardIt >
	constexpr typename std::iterator_traits<ForwardIt>::difference_type 
		distance_up_to_n(ForwardIt first, ForwardIt last, typename std::iterator_traits<ForwardIt>::difference_type n)
		noexcept(noexcept(impl::distance_up_to_n(first, last, n, typename std::iterator_traits<ForwardIt>::iterator_category()))) {
		return impl::distance_up_to_n(first, last, n, typename std::iterator_traits<ForwardIt>::iterator_category());
	}

	namespace impl {
		template< class ForwardIt>
		constexpr std::pair<ForwardIt, typename std::iterator_traits<ForwardIt>::difference_type> 
			next_up_to_n(ForwardIt first, ForwardIt last, typename std::iterator_traits<ForwardIt>::difference_type n, std::forward_iterator_tag)
			noexcept(noexcept(first != last)
				&& std::is_nothrow_constructible_v<ForwardIt>
				&& noexcept(++first)) {
			typename std::iterator_traits<ForwardIt>::difference_type d = 0;
			while (first != last && d < n) {
				++first;
				++d;
			}
			return {first, d};
		}
		template< class ForwardIt>
		constexpr std::pair<ForwardIt, typename std::iterator_traits<ForwardIt>::difference_type>
			next_up_to_n(ForwardIt first, ForwardIt last, typename std::iterator_traits<ForwardIt>::difference_type n, std::random_access_iterator_tag)
			noexcept(noexcept(last - first < n)
				&& std::is_nothrow_constructible_v<ForwardIt>
				&& noexcept(first + 1)) {
			typename std::iterator_traits<ForwardIt>::difference_type d = ((last - first) < n) ? (last - first) : n;
			return {first + d, d};
		}
	}
	using std::distance;
	template< class ForwardIt >
	constexpr ForwardIt next_up_to_n(ForwardIt first, ForwardIt last, typename std::iterator_traits<ForwardIt>::difference_type n)
		noexcept(noexcept(impl::next_up_to_n(first, last, n, std::iterator_traits<ForwardIt>::tag()))) {
		return impl::next_up_to_n(first, last, n, std::iterator_traits<ForwardIt>::tag());
	}
}