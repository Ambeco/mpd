#pragma once
#include <iterator>
#include <utility>

namespace mpd {
	template< class T >
	void destroy_at(T* p) {
		p->~T();
	}
	template< class T, std::size_t N >
	void destroy_at(T(*p)[N]) {
		for (auto& elem : *p)
			destroy_at(std::addressof(elem));
	}
	template< class ForwardIt >
	void destroy(ForwardIt first, ForwardIt last) {
		while (first != last)
			destroy_at(std::addressof(*--last));
	}
	template< class ForwardIt >
	void uninitialized_default_construct(ForwardIt first, ForwardIt last) {
		using Value = typename std::iterator_traits<ForwardIt>::value_type;
		ForwardIt current = first;
		try {
			for (; current != last; ++current) {
				::new (static_cast<void*>(std::addressof(*current))) Value;
			}
		}
		catch (...) {
			destroy(first, current);
			throw;
		}
	}
	template< class ForwardIt >
	void uninitialized_value_construct_n(ForwardIt first, std::size_t count) {
		using Value = typename std::iterator_traits<ForwardIt>::value_type;
		ForwardIt current = first;
		try {
			for (std::size_t i = 0; i < count; ++i) {
				::new (static_cast<void*>(std::addressof(*current))) Value();
			}
		}
		catch (...) {
			destroy(first, current);
			throw;
		}
	}
	template< class InputIt, class Size, class OutputIt >
	OutputIt copy_n(InputIt first, Size count, OutputIt result) {
		if (count > 0) {
			*result++ = *first;
			for (Size i = 1; i < count; ++i) {
				*result++ = *++first;
			}
		}
		return result;
	}
	template< class InputIt, class Size, class OutputIt >
	OutputIt move_n(InputIt first, Size count, OutputIt result) {
		if (count > 0) {
			*result++ = *first;
			for (Size i = 1; i < count; ++i) {
				*result++ = std::move(*++first);
			}
		}
		return result;
	}
	template< class InputIt, class OutputIt >
	OutputIt uninitialized_move(InputIt first, InputIt last, OutputIt result) {
		using Value = typename std::iterator_traits<OutputIt>::value_type;
		InputIt current = first;
		try {
			for (; current != last; ++current) {
				::new (static_cast<void*>(std::addressof(*current))) Value(std::move(*first));
			}
		}
		catch (...) {
			destroy(first, current);
			throw;
		}
	}
	//similar to std::copy, but takes a count for the destination for early stopping
	template< class InputIt, class Size, class ForwardIt >
	std::pair< InputIt, ForwardIt> copy_up_to_n(InputIt src_first, InputIt src_last, Size max, ForwardIt dest_first) {
		while (max && src_first != src_last) {
			*dest_first = *src_first;
			++src_first;
			++dest_first;
			--max;
		}
		return { src_first, dest_first };
	}
	template< class InputIt, class Size, class ForwardIt >
	std::pair< InputIt, ForwardIt> uninitialized_copy_up_to_n(InputIt src_first, InputIt src_last, Size max, ForwardIt dest_first) {
		using destT = decltype(*dest_first);
		ForwardIt init_dest = dest_first;
		try {
			while (max && src_first != src_last) {
				new(&*dest_first)destT(*src_first);
				++src_first;
				++dest_first;
				--max;
			}
			return { src_first, dest_first };
		}
		catch (...) {
			destroy(init_dest, dest_first);
			throw;
		}
	}
	//std::copy_backward_n is simply missing from the standard library
	template<class InBidirIt, class Size, class OutBidirIt >
	OutBidirIt copy_backward_n(InBidirIt src_last, Size count, OutBidirIt dest_last) {
		for (Size i = 0; i < count; ++i)
			*(--dest_last) = *(--src_last);
		return dest_last;
	}
	template<class InBidirIt, class Size, class OutBidirIt >
	OutBidirIt uninitialized_copy_backward_n(InBidirIt src_last, Size count, OutBidirIt dest_last) {
		using T = typename std::iterator_traits<dest_last>::value_type;
		OutBidirIt init_dest = dest_last;
		Size i = 0;
		try {
			for (Size i = 0; i < count; ++i)
				new(&*--dest_last)T(*--src_last);
			return dest_last;
		}
		catch (...) {
			destroy(dest_last, init_dest);
			throw;
		}
	}
	template<class InBidirIt, class Size, class OutBidirIt >
	OutBidirIt move_backward_n(InBidirIt src_last, Size count, OutBidirIt dest_last) {
		for (Size i = 0; i < count; ++i)
			*(--dest_last) = std::move(*(--src_last));
		return dest_last;
	}
	template<class InBidirIt, class Size, class OutBidirIt >
	OutBidirIt uninitialized_move_backward_n(InBidirIt src_last, Size count, OutBidirIt dest_last) {
		using T = typename std::iterator_traits<dest_last>::value_type;
		OutBidirIt init_dest = dest_last;
		Size i = 0;
		try {
			for (Size i = 0; i < count; ++i)
				new(&*--dest_last)T(std::move(*--src_last));
			return dest_last;
		}
		catch (...) {
			destroy(dest_last, init_dest);
			throw;
		}
	}

	//similar to std::strlen_s, but works on arbitrary character types
	template<class charT>
	std::size_t strnlen_s_algorithm(const charT* ptr, std::size_t max) {
		std::size_t len = 0;
		while (*ptr && len < max) {
			++ptr;
			++len;
		}
		return len;
	}

	//similar to std::distance, but takes a count for the destination for early stopping
	template<class ForwardIt>
	typename std::iterator_traits<ForwardIt>::difference_type distance_up_to_n(ForwardIt first, ForwardIt last, typename std::iterator_traits<ForwardIt>::difference_type max, std::forward_iterator_tag)
	{
		typename std::iterator_traits<ForwardIt>::difference_type distance = 0;
		while (first != last && distance < max) {
			++first;
			++distance;
		}
		return distance;
	}
	template<class RandIt>
	typename std::iterator_traits<RandIt>::difference_type distance_up_to_n(RandIt first, RandIt last, typename std::iterator_traits<RandIt>::difference_type max, std::random_access_iterator_tag)
	{
		return std::min(last - first, max);
	}
	template<class ForwardIt>
	typename std::iterator_traits<ForwardIt>::difference_type distance_up_to_n(ForwardIt first, ForwardIt last, typename std::iterator_traits<ForwardIt>::difference_type max)
	{
		return distance_up_to_n(first, last, max, typename std::iterator_traits<ForwardIt>::iterator_category{});
	}
}