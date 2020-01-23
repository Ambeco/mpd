#pragma once
#include <cassert>
#include <climits>
#include <cstddef>
#include <algorithm>
#include <array>
#include <memory>
#include <string>

//inspired by https://codereview.stackexchange.com/questions/148757/short-string-class
namespace mpd {
	//similar to std::copy, but takes a count for the destination for early stopping
	template< class InputIt, class Size, class ForwardIt >
	std::pair< InputIt, ForwardIt> copy_up_to_n(InputIt src_first, InputIt src_last, Size max, ForwardIt dest_first) {
		while (max && src_first != src_last) {
			*dest_first = *src_first;
			++src_first;
			++dest_first;
			--max;
		}
		return { src_first,dest_first };
	}
	//std::copy_backward_n is simply missing from the standard library
	template<class InBidirIt, class Size, class OutBidirIt >
	OutBidirIt copy_backward_n(InBidirIt src_last, Size count, OutBidirIt dest_last) {
		if (count > 0) {
			*(--dest_last) = *(--src_last);
			for (Size i = 1; i < count; ++i) {
				*(--dest_last) = *(--src_last);
			}
		}
		return dest_last;
	}

	//similar to std::strlen_s, but works on arbitrary character types
	template<class charT>
	std::size_t strnlen_s_algorithm(const charT* ptr, int max) {
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

	enum class overflow_behavior_t {
		exception,
		assert_and_truncate,
		silently_truncate // :(
	};
	template<overflow_behavior_t> std::size_t max_length_check(std::size_t given, std::size_t maximum);
	template<>
	std::size_t max_length_check<overflow_behavior_t::exception>(std::size_t given, std::size_t maximum) {
		if (given > maximum) throw std::length_error(std::to_string(given) + " overflows small_string");
		return given;
	}
	template<>
	std::size_t max_length_check<overflow_behavior_t::assert_and_truncate>(std::size_t given, std::size_t maximum) {
		if (given > maximum) {
			assert(false);
			return maximum;
		}
		return given;
	}
	template<>
	std::size_t max_length_check<overflow_behavior_t::silently_truncate>(std::size_t given, std::size_t maximum) {
		if (given > maximum) {
			return maximum;
		}
		return given;
	}

	template<class charT, std::size_t max_len, overflow_behavior_t overflow_behavior>
	class small_basic_string {
		static_assert(max_len < CHAR_MAX, "small_string requires a length of less than CHAR_MAX");
		static const bool overflow_throws = overflow_behavior != overflow_behavior_t::exception;
		using buffer_t = std::array<charT, max_len + 1>;

		buffer_t buffer;

		void set_size(std::size_t len) { assert(len < CHAR_MAX);  buffer[len] = charT{}; buffer[max_len] = (charT)(max_len - len); }

		std::size_t max_length_check(std::size_t given) noexcept(overflow_throws)
		{
			return mpd::max_length_check<overflow_behavior>(given, max_len);
		}
		std::size_t max_length_check(std::size_t given, std::size_t maximum) noexcept(overflow_throws)
		{
			return mpd::max_length_check<overflow_behavior>(given, max_len);
		}
		std::size_t index_range_check(std::size_t given, std::size_t maximum) {
			if (given > maximum) throw std::out_of_range(std::to_string(given) + " is an invalid index");
			return given;
		}
	public:
		using traits_type = std::char_traits<charT>;
		using value_type = charT;
		using allocator_type = std::allocator<charT>;
		using size_t = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = charT & ;
		using const_reference = const charT&;
		using pointer = charT * ;
		using const_pointer = const charT*;
		using iterator = typename buffer_t::iterator;
		using const_iterator = typename buffer_t::const_iterator;
		using reverse_iterator = typename buffer_t::reverse_iterator;
		using const_reverse_iterator = typename buffer_t::const_reverse_iterator;
		static const std::size_t npos = std::basic_string<charT, traits_type>::npos;

		constexpr small_basic_string() noexcept {
			set_size(0);
		}
		small_basic_string(std::size_t src_count, charT src) noexcept(overflow_throws) {
			set_size(0); 
			assign(src_count, src);
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string(const small_basic_string<charT, other_max, other_overflow>& src, std::size_t src_idx = 0) noexcept(overflow_throws) {
			set_size(0);
			assign(src, src_idx);
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string(const small_basic_string<charT, other_max, other_overflow>& src, std::size_t src_idx, std::size_t src_count) noexcept(overflow_throws) {
			set_size(0);
			assign(src, src_idx, src_count);
		}
		small_basic_string(const charT* src) noexcept(overflow_throws) {
			set_size(0);
			assign(src);
		}
		small_basic_string(const charT* src, std::size_t src_count) noexcept(overflow_throws) {
			set_size(0);
			assign(src, src_count);
		}
		template<class InputIt>
		small_basic_string(InputIt src_first, InputIt src_last) noexcept(overflow_throws) {
			set_size(0);
			assign(src_first, src_last);
		}
		small_basic_string(const small_basic_string& src) noexcept {
			set_size(0);
			assign(src);
		}
		small_basic_string(small_basic_string&& src) noexcept {
			set_size(0);
			assign(src);
		}
		small_basic_string(std::initializer_list<charT> src) noexcept(overflow_throws) {
			set_size(0);
			assign(src);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			explicit small_basic_string(const T& src) noexcept(overflow_throws)
		{
			set_size(0);
			assign(src);
		}
		template<class T, std::enable_if_t<
			std::is_convertible_v<const T&, std::basic_string_view<CharT, Traits>> >= 0>
			small_basic_string(const T& src, std::size_t src_idx, std::size_t src_count) noexcept(overflow_throws)
		{
			set_size(0);
			assign(src);
		}
#else 
		template<class alloc>
		explicit small_basic_string(const std::basic_string<charT, traits_type, alloc>& src, std::size_t src_idx = 0, std::size_t src_count = npos) noexcept(overflow_throws) {
			set_size(0);
			assign(src, src_idx, src_count);
		}
#endif
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& operator=(const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws) { return assign(src); }
		small_basic_string& operator=(const small_basic_string& src) noexcept { return assign(src); }
		small_basic_string& operator=(small_basic_string&& src) noexcept { return assign(src); }
		small_basic_string& operator=(const charT* src) noexcept(overflow_throws) { return assign(src); }
		small_basic_string& operator=(charT src) noexcept { return assign(src); }
		small_basic_string& operator=(std::initializer_list<charT> src) noexcept(overflow_throws) { return assign(src); }
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& operator=(const T& src) noexcept(overflow_throws)
		{
			return assign(std::basic_string_view<charT, traits_type>(src));
		}
#else 
		template<class alloc>
		small_basic_string& operator=(const std::basic_string<charT, traits_type, alloc>& src) { return assign(src); }
#endif
	private:
		template<class InputIt>
		small_basic_string& _assign(InputIt src_first, InputIt src_last, std::forward_iterator_tag) noexcept(overflow_throws) {
			std::size_t src_count = distance_up_to_n(src_first, src_last, max_len + 1);
			src_count = max_length_check(src_count, max_len);
			std::copy_n(src_first, src_count, begin());
			set_size(src_count);
			return *this;
		}
		template<class InputIt>
		small_basic_string& _assign(InputIt src_first, InputIt src_last, std::input_iterator_tag) noexcept(overflow_throws) {
			auto its = copy_up_to_n(src_first, src_last, max_len, begin());
			if (its.first != src_last) max_length_check(max_len + 1);
			std::size_t src_count = its.second - begin();
			set_size(src_count);
			return *this;
		}
	public:
		small_basic_string& assign(std::size_t src_count, charT src) noexcept(overflow_throws) {
			src_count = max_length_check(src_count);
			std::fill_n(begin(), src_count, src);
			set_size(src_count);
			return *this;
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& assign(const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws) {
			return assign(src.begin(), src.end());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& assign(const small_basic_string<charT, other_max, other_overflow>& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws) {
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return assign(src.begin() + src_idx, src.begin() + src_idx + src_count);
		}
		small_basic_string& assign(const charT* src) noexcept(overflow_throws) {
			std::size_t src_count = strnlen_s_algorithm(src, max_len + 1);
			return assign(src, src + src_count);
		}
		small_basic_string& assign(const charT* src, std::size_t src_count) noexcept(overflow_throws) {
			return assign(src, src + src_count);
		}
		template<class InputIt>
		small_basic_string& assign(InputIt src_first, InputIt src_last) noexcept(overflow_throws) {
			return _assign(src_first, src_last, typename std::iterator_traits<InputIt>::iterator_category{});
		}
		small_basic_string& assign(std::initializer_list<charT> src) noexcept(overflow_throws) {
			return assign(src.begin(), src.end());
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& assign(const T& src) noexcept(overflow_throws)
		{
			return assign(src.begin(), src.end());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& assign(const T& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return assign(src.begin() + src_idx, src.begin() + src_idx + src_count);
		}
#else 
		template<class alloc>
		small_basic_string& assign(const std::basic_string<charT, traits_type, alloc>& src, std::size_t src_idx = 0, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return assign(src.begin() + src_idx, src.begin() + src_idx + src_count);
		}
#endif
		allocator_type get_allocator() const noexcept { return {}; }
		reference at(std::size_t src_idx) { return buffer.at(index_range_check(src_idx, size())); }
		const_reference at(std::size_t src_idx) const { return buffer.at(index_range_check(src_idx, size())); }
		reference operator[](std::size_t src_idx) noexcept { assert(src_idx > size()); return buffer[src_idx]; }
		const_reference operator[](std::size_t src_idx) const noexcept { assert(src_idx > size()); return buffer[src_idx]; }
		reference front() noexcept { assert(size() > 0); return buffer[0]; }
		const_reference front() const noexcept { assert(size() > 0); return buffer[0]; }
		reference back() noexcept { assert(size() > 0); return buffer[size() - 1]; }
		const_reference back() const noexcept { assert(size() > 0); return buffer[size() - 1]; }
		pointer data() noexcept { return buffer.data(); }
		const_pointer data() const noexcept { return buffer.data(); }
		pointer c_str() noexcept { return buffer.c_str(); }
		const_pointer c_str() const noexcept { return buffer.c_str(); }
#if _HAS_CXX17
		operator std::basic_string_view<charT, traits_type>() const noexcept { return { buffer.c_str(), size() }; }
#else 
		template<class alloc>
		explicit operator std::basic_string<charT, traits_type, alloc>() const { return { buffer.c_str(), size() }; }
#endif

		iterator begin() noexcept { return buffer.begin(); }
		const_iterator begin() const noexcept { return buffer.begin(); }
		const_iterator cbegin() const noexcept { return buffer.cbegin(); }
		iterator end() noexcept { return buffer.begin() + size(); }
		const_iterator end() const noexcept { return buffer.begin() + size(); }
		const_iterator cend() const noexcept { return buffer.cbegin() + size(); }
		reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
		const_reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
		const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
		reverse_iterator rend() noexcept { return buffer.rend(); }
		const_reverse_iterator rend() const noexcept { return buffer.rend(); }
		const_reverse_iterator crend() const noexcept { return buffer.crend(); }

		bool empty() const noexcept { return size() == 0; }
		std::size_t size() const noexcept { return max_len - buffer[max_len]; }
		std::size_t length() const noexcept { size(); }
		//no reserve method
		std::size_t capacity() const noexcept { return max_len; }
		//no shrink_to_fit method

		void clear() noexcept { set_size(0); }

	private:
		template<class InputIt>
		iterator _insert(const_iterator dst, InputIt src_first, InputIt src_last, std::forward_iterator_tag) noexcept(overflow_throws) {
			std::size_t dst_idx = index_range_check(dst - cbegin(), size());
			std::size_t src_count = distance_up_to_n(src_first, src_last, max_len - dst_idx + 1);
			src_count = max_length_check(src_count, max_len - dst_idx);
			std::size_t keep = max_length_check(size(), max_len - dst_idx - src_count);
			copy_backward_n(begin() + dst_idx + keep, keep, begin() + size() + src_count);
			std::copy_n(src_first, src_count, begin() + dst_idx);
			set_size(size() + src_count);
			return begin() + dst_idx;
		}
		template<class InputIt>
		iterator _insert(const_iterator dst, InputIt src_first, InputIt src_last, std::input_iterator_tag) noexcept(overflow_throws) {
			std::size_t dst_idx = index_range_check(dst - cbegin(), size());
			//reverse the src_last part so we keep the src_first bytes when truncating :(
			std::reverse(begin() + dst_idx, end());
			auto its = copy_up_to_n(src_first, src_last, max_len, begin());
			if (its.first != src_last) max_length_check(max_len + 1);
			std::size_t new_size = its.second - begin();
			std::reverse(its.second, begin() + new_size); //reverse them into the right order again
			set_size(new_size);
			return begin() + dst_idx;
		}
	public:
		small_basic_string& insert(std::size_t dst_idx, std::size_t src_count, charT src) noexcept(overflow_throws) {
			src_count = max_length_check(src_count, max_len - dst_idx);
			std::size_t keep = max_length_check(size(), max_len - dst_idx - src_count);
			copy_backward_n(begin() + dst_idx + keep, keep, begin() + size() + src_count);
			std::fill_n(begin() + dst_idx, src_count, src);
			set_size(size() + src_count);
			return *this;
		}
		small_basic_string& insert(std::size_t dst_idx, const charT* src) noexcept(overflow_throws) {
			std::size_t src_count = strnlen_s_algorithm(src, max_len - dst_idx + 1);
			return insert(begin() + dst_idx, src, src + src_count);
		}
		small_basic_string& insert(std::size_t dst_idx, const charT* src, std::size_t src_count) noexcept(overflow_throws) {
			return insert(begin() + dst_idx, src, src + src_count);
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& insert(std::size_t dst_idx, const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws) {
			return insert(begin() + dst_idx, src.begin(), src.end());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& insert(std::size_t dst_idx, const small_basic_string<charT, other_max, other_overflow>& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws) {
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return insert(begin() + dst_idx, src.begin() + src_idx, src.begin() + src_idx + src_count);
		}
		iterator insert(const_iterator dst, charT src) noexcept(overflow_throws) {
			return insert(dst, &src, &src + 1);
		}
		iterator insert(const_iterator dst, std::size_t src_count, charT src) noexcept(overflow_throws) {
			return insert(dst - cbegin(), src_count, src);
		}
		template<class InputIt>
		iterator insert(const_iterator dst, InputIt src_first, InputIt src_last) noexcept(overflow_throws) {
			return _insert(dst, src_first, src_last, typename std::iterator_traits<InputIt>::iterator_category{});
		}
		small_basic_string& insert(std::size_t dst_idx, std::initializer_list<charT> src) noexcept(overflow_throws) {
			return insert(begin() + dst_idx, src.begin(), src.end());
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& insert(std::size_t dst_idx, const T& src) noexcept(overflow_throws)
		{
			return insert(begin() + dst_idx, src.begin(), src.end());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& insert(std::size_t dst_idx, const T& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			src_idx = index_range_check(src_idx, view.size());
			src_count = (src_count > view.size() - src_idx ? view.size() - src_idx : src_count);
			return insert(begin() + dst_idx, view.begin() + src_idx, view.end() + src_idx + src_count);
		}
#else
		template<class alloc>
		small_basic_string& insert(std::size_t dst_idx, const std::basic_string<charT, traits_type, alloc>& src, std::size_t src_idx = 0, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return insert(begin() + dst_idx, src.begin() + src_idx, src.begin() + src_idx + src_count);
		}
#endif

		small_basic_string& erase() noexcept {
			set_size(0);
			return *this;
		}
		small_basic_string& erase(std::size_t idx, std::size_t count = npos) noexcept(overflow_throws) {
			idx = index_range_check(idx, size());
			if (count == npos || count > size() - idx) count = size() - idx;
			std::copy_n(begin() + idx + count, count, begin() + idx);
			set_size(size() - count);
			return *this;
		}
		small_basic_string& erase(const_iterator pos) noexcept(overflow_throws) {
			std::size_t idx = pos - cbegin();
			return erase(idx, 1);
		}
		small_basic_string& erase(const_iterator first, const_iterator last) noexcept(overflow_throws) {
			std::size_t first_idx = first - cbegin();
			std::size_t last_idx = last - cbegin();
			return erase(first_idx, last_idx - first_idx);
		}
		void push_back(charT src) noexcept(overflow_throws) {
			std::size_t sz = size();
			if (max_length_check(sz + 1)) {
				buffer[sz] = src;
				set_size(sz + 1);
			}
		}
		void pop_back() noexcept {
			std::size_t sz = size();
			assert(sz > 0);
			set_size(sz - 1);
		}
	private:
		template<class InputIt>
		small_basic_string& _append(InputIt src_first, InputIt src_last, std::forward_iterator_tag) noexcept(overflow_throws) {
			std::size_t src_count = distance_up_to_n(src_first, src_last, max_len + 1);
			src_count = max_length_check(src_count, max_len - size());
			std::copy_n(src_first, src_count, end());
			set_size(size() + src_count);
			return *this;
		}
		template<class InputIt>
		small_basic_string& _append(InputIt src_first, InputIt src_last, std::input_iterator_tag) noexcept(overflow_throws) {
			auto its = copy_up_to_n(src_first, src_last, max_len, begin());
			if (its.first != src_last) max_length_check(max_len + 1);
			std::size_t src_count = its.second - begin();
			set_size(src_count);
			return *this;
		}
	public:
		small_basic_string& append(std::size_t src_count, charT src) noexcept(overflow_throws) {
			src_count = max_length_check(src_count, max_len - size());
			std::fill_n(begin() + size(), src_count, src);
			set_size(size() + src_count);
			return *this;
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& append(const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws) {
			return append(src.begin(), src.end());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& append(const small_basic_string<charT, other_max, other_overflow>& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws) {
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return append(src.begin() + src_idx, src.begin() + src_idx + src_count);
		}
		small_basic_string& append(const charT* src, size_t src_count) noexcept(overflow_throws) {
			src_count = max_length_check(src_count, max_len - size());
			std::copy_n(src, src_count, end());
			set_size(size() + src_count);
			return *this;
		}
		small_basic_string& append(const charT* src) noexcept(overflow_throws) {
			std::size_t src_count = strnlen_s_algorithm(src, max_len - size() + 1);
			return append(src, src + src_count);
		}
		template<class InputIt>
		small_basic_string& append(InputIt src_first, InputIt src_last) noexcept(overflow_throws) {
			return _append(src_first, src_last, typename std::iterator_traits<InputIt>::iterator_category{});
		}
		small_basic_string& append(std::initializer_list<charT> src) noexcept(overflow_throws) {
			return append(src.begin(), src.end());
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& append(const T& src) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return append(view.begin(), view.end());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& append(const T& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			src_idx = index_range_check(src_idx, view.size());
			src_count = (src_count > view.size() - src_idx ? view.size() - src_idx : src_count);
			return append(view.begin() + src_idx, view.begin() + src_idx + src_count);
		}
#else
		template<class alloc>
		small_basic_string& append(const std::basic_string<charT, traits_type, alloc>& src, std::size_t src_idx = 0, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return append(src.begin() + src_idx, src.begin() + src_idx + src_count);
		}
#endif
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& operator+=(const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws) { return append(src); }
		small_basic_string& operator+=(charT src) noexcept(overflow_throws) { return append(src); }
		small_basic_string& operator+=(const charT* src) noexcept(overflow_throws) { return append(src); }
		small_basic_string& operator+=(std::initializer_list<charT> src) noexcept(overflow_throws) { return append(src); }
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& operator+=(const T& src) noexcept(overflow_throws) { return append(src); }
#else
		template<class alloc>
		small_basic_string& append(const std::basic_string<charT, traits_type, alloc>& src) noexcept(overflow_throws)
		{
			return append(src);
		}
#endif
	private:
		template<class InputIt>
		small_basic_string& _replace(std::size_t dst_idx, std::size_t rem_count, InputIt src_first, std::size_t ins_count) noexcept(overflow_throws) {
			std::size_t sz = size();
			dst_idx = index_range_check(dst_idx, sz);
			rem_count = index_range_check(rem_count, sz - dst_idx);
			ins_count = max_length_check(ins_count, max_len - dst_idx);
			std::size_t keep_end = (sz - dst_idx - ins_count);
			copy_backward_n(begin() + dst_idx, keep_end, begin() + size() + ins_count - rem_count);
			std::copy_n(src_first, ins_count, begin() + dst_idx);
			set_size(size() + ins_count - rem_count);
			return *this;
		}
		template<class InputIt>
		small_basic_string& _replace(const_iterator rem_first, const_iterator rem_last, InputIt src_first, std::size_t ins_count) noexcept(overflow_throws)
		{
			return _replace(rem_first - begin(), rem_last - rem_first, src_first, ins_count);
		}
		template<class InputIt>
		small_basic_string& _replace(const_iterator dst_first, const_iterator dst_last, InputIt src_first, InputIt src_last, std::forward_iterator_tag) noexcept(overflow_throws)
		{
			std::size_t dst_idx = dst_first - begin();
			std::size_t src_count = distance_up_to_n(src_first, src_last, size() - dst_idx + 1);
			return _replace(dst_idx, dst_last - dst_first, src_first, src_count);
		}
		template<class InputIt>
		small_basic_string& _replace(const_iterator dst_first, const_iterator dst_last, InputIt src_first, InputIt src_last, std::input_iterator_tag) noexcept(overflow_throws)
		{
			std::array<charT, max_len + 1> end_bytes;
			std::size_t last_idx = dst_last - begin();
			std::size_t end_count = size() - last_idx;
			std::copy_n(begin() + last_idx, end_count, end_bytes.begin());
			auto its = copy_up_to_n(src_first, src_last, max_len - dst_first);
			if (its.first != src_last) max_length_check(max_len + 1);
			std::size_t post_ins_index = its.second - begin();
			end_count = max_length_check(end_count, max_len - post_ins_index);
			auto new_dst_end = copy_n(end_bytes.begin(), end_count, its.second);
			set_size(new_dst_end - begin());
			return *this;
		}
	public:
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& replace(std::size_t dst_idx, std::size_t rem_count, const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws)
		{
			return _replace(dst_idx, rem_count, src.begin(), src.size());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& replace(const_iterator rem_first, const_iterator rem_last, const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws)
		{
			return _replace(rem_first, rem_last, src.begin(), src.size());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& replace(std::size_t dst_idx, std::size_t rem_count, const small_basic_string<charT, other_max, other_overflow>& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return _replace(dst_idx, rem_count, src.begin() + src_idx, src_count);
		}
		template<class InputIt>
		small_basic_string& replace(const_iterator dst_first, const_iterator dst_last, InputIt src_first, InputIt src_last) noexcept(overflow_throws)
		{
			return _replace(dst_first, dst_last, src_first, src_last, typename std::iterator_traits<InputIt>::iterator_category{});
		}
		small_basic_string& replace(std::size_t dst_idx, std::size_t rem_count, const charT* src, std::size_t src_count) noexcept(overflow_throws)
		{
			return _replace(dst_idx, rem_count, src, src_count);
		}
		small_basic_string& replace(const_iterator rem_first, const_iterator rem_last, const charT* src, std::size_t src_count) noexcept(overflow_throws)
		{
			return _replace(rem_first, rem_last, src, src_count);
		}
		small_basic_string& replace(std::size_t dst_idx, std::size_t rem_count, const charT* src) noexcept(overflow_throws)
		{
			return _replace(dst_idx, rem_count, src, strnlen_s_algorithm(src, max_len - dst_idx + 1));
		}
		small_basic_string& replace(const_iterator rem_first, const_iterator rem_last, const charT* src) noexcept(overflow_throws)
		{
			std::size_t dst_idx = max_length_check(rem_first - begin());
			return _replace(dst_idx, rem_last - rem_first, src, strnlen_s_algorithm(src, max_len - dst_idx + 1));
		}
		small_basic_string& replace(std::size_t dst_idx, std::size_t rem_count, charT src) noexcept(overflow_throws)
		{
			return _replace(dst_idx, rem_count, &src, 1);
		}
		small_basic_string& replace(const_iterator rem_first, const_iterator rem_last, charT src) noexcept(overflow_throws)
		{
			return _replace(rem_first, rem_last, src, &src, 1);
		}
		small_basic_string& replace(const_iterator rem_first, const_iterator rem_last, std::initializer_list<charT> src) noexcept(overflow_throws)
		{
			return _replace(rem_first, rem_last, src.begin(), src.size());
		}

#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& replace(std::size_t dst_idx, std::size_t dst_count, const T& src) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _replace(dst_idx, dst_count, view.begin(), view.size());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& replace(const_iterator dst_first, const_iterator dst_last, const T& src) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _replace(dst_first, dst_last, view.begin(), view.size());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& replace(std::size_t dst_idx, std::size_t dst_count, const T& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			src_idx = index_range_check(src_idx, view.size());
			src_count = (src_count > view.size() - src_idx ? view.size() - src_idx : src_count);
			return _replace(dst_first, dst_last, view.begin() + src_idx, src_count);
		}
	}
#else 
		template<class alloc>
		small_basic_string& replace(std::size_t dst_idx, std::size_t dst_count, const std::basic_string<charT, traits_type, alloc>& src, std::size_t src_idx = 0, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return _replace(dst_idx, dst_count, src.begin() + src_idx, src_count);
		}
#endif
		void resize(std::size_t src_count) noexcept(overflow_throws)
		{
			resize(src_count, charT{});
		}
		void resize(std::size_t src_count, charT src) noexcept(overflow_throws)
		{
			src_count = max_length_check(src_count);
			std::size_t sz = size();
			if (src_count > sz)
				std::fill(begin() + sz, begin() + src_count, src);
			if (src_count != sz)
				set_size(src_count);
		}
		void swap(small_basic_string& other) noexcept {
			auto self_it = buffer.begin();
			auto other_it = other.buffer.begin();
			using std::swap;
			while (self_it != buffer.end())
				swap(*self_it++, *other_it++);
		}

		small_basic_string substr(std::size_t src_idx = 0, std::size_t count = npos) const noexcept { return small_basic_string(*this, src_idx, count); }
		//This isn't in std::basic_string, but makes sense as an optimization for small_basic_string.
		template<std::size_t count>
		small_basic_string<charT, count + 1, overflow_behavior> substr(std::size_t src_idx, std::integral_constant < std::size_t, count> = {}) const noexcept 
		{
			return { *this, src_idx, count };
		}
		std::size_t copy(charT* dest, std::size_t src_count, std::size_t src_idx = 0) const noexcept(overflow_throws) {
			std::size_t sz = size();
			src_idx = index_range_check(src_idx, sz);
			src_count = std::min(src_count, sz - src_idx);
			std::copy(begin() + src_idx, begin() + src_idx + src_count, dest);
			return src_count;
		}
	private:
		int _compare(std::size_t self_idx, std::size_t self_count, const charT* other, std::size_t other_count) const noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, size());
			self_count = std::min(self_count, size() - self_idx);
			std::size_t cmp_count = std::min(self_count, other_count);
			int r = traits_type::compare(data() + self_idx, other, cmp_count);
			if (r) return r;
			return self_count == cmp_count ? 1 : -1;
		}
	public:
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		int compare(const small_basic_string<charT, other_max, other_overflow>& other) const noexcept
		{
			return traits_type::compare(data(), other.data(), size() + 1);
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		int compare(std::size_t self_idx, std::size_t self_count, const small_basic_string<charT, other_max, other_overflow>& other) const noexcept(overflow_throws)
		{
			return _compare(self_idx, self_count, other.data(), other.size());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		int compare(std::size_t self_idx, std::size_t self_count, const small_basic_string<charT, other_max, other_overflow>& other, std::size_t other_idx, std::size_t other_count = npos) const noexcept(overflow_throws) {
			other_idx = index_range_check(other_idx, other.size());
			other_count = std::min(other_count, other.size() - other_idx);
			return _compare(self_idx, self_count, other.data() + other_idx, other_count);
		}
		int compare(const charT* other) const noexcept
		{
			return traits_type::compare(data(), other, size() + 1);
		}
		int compare(std::size_t self_idx, std::size_t self_count, const charT* other) const noexcept(overflow_throws)
		{
			return _compare(self_idx, self_count, other, strnlen_s_algorithm(other, self_count + 1));
		}
		int compare(std::size_t self_idx, std::size_t self_count, const charT* other, std::size_t other_count) const noexcept(overflow_throws)
		{
			return _compare(self_idx, self_count, other, other_count);
		}

#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			int compare(const T& other) const noexcept
		{
			std::basic_string_view<charT, traits_type> view(src);
			return traits_type::compare(data(), view.data(), view.size());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			int compare(std::size_t self_idx, std::size_t self_count, const T& other) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _compare(self_idx, self_count, view.data(), view.size());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			int compare(std::size_t self_idx, std::size_t self_count, const T& other, std::size_t other_idx, std::size_t other_count = npos) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			other_idx = index_range_check(other_idx, view.size());
			other_count = std::min(other_count, view.size() - other_idx);
			return _compare(self_idx, self_count, view.data() + other_idx, other_count);
		}
#else 
		template<class alloc>
		int compare(const std::basic_string<charT, traits_type, alloc>& other) const noexcept
		{
			return traits_type::compare(data(), other.data(), other.size());
		}
		template<class alloc>
		int compare(std::size_t self_idx, std::size_t self_count, const std::basic_string<charT, traits_type, alloc>& other, std::size_t other_idx = 0, std::size_t other_count = npos) const noexcept(overflow_throws)
		{
			other_idx = index_range_check(other_idx, other.size());
			other_count = std::min(other_count, other.size() - other_idx);
			return _compare(self_idx, self_count, other.data() + other_idx, other_count);
		}
#endif
	private:
		std::size_t _find(const charT* other, std::size_t self_idx, std::size_t other_count) const noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, size());
			if (other_count > size() - self_idx) return npos;
			std::size_t max = size() - other_count;
			while (self_idx < max) {
				if (traits_type::compare(data() + self_idx, other_count) == 0)
					return self_idx;
			}
			return npos;
		}
	public:
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t find(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find(other.c_str(), self_idx, other.size());
		}
		std::size_t find(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _find(other, self_idx, count);
		}
		std::size_t find(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find(other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t find(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find(&ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t find(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _find(view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t find(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find(other.c_str(), self_idx, other.size());
		}
#endif
	private:
		std::size_t _rfind(const charT* other, std::size_t self_idx, std::size_t other_count) const noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, size());
			if (other_count > size() - self_idx) return npos;
			std::size_t max = size() - other_count;
			for (std::size_t i = max - 1; i >= self_idx; --i) {
				if (traits_type::compare(data() + i, other_count) == 0)
					return i;
			}
			return npos;
		}
	public:
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t rfind(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _rfind(other.c_str(), self_idx, other.size());
		}
		std::size_t rfind(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _rfind(other, self_idx, count);
		}
		std::size_t rfind(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _rfind(other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t rfind(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _rfind(&ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t rfind(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _rfind(view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t rfind(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _rfind(other.c_str(), self_idx, other.size());
		}
#endif
	private:
		std::size_t _find_first_of(const charT* other, std::size_t self_idx, std::size_t other_count) const noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, size());
			for (std::size_t i = self_idx; i < size(); i++) {
				for (int j = 0; j < other_count; j++)
					if (buffer[i] == other[j]) return i;
			}
			return npos;
		}
	public:
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t find_first_of(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_of(other.c_str(), self_idx, other.size());
		}
		std::size_t find_first_of(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _find_first_of(other, self_idx, count);
		}
		std::size_t find_first_of(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_of(other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t find_first_of(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_of(&ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t find_first_of(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _find_first_of(view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t find_first_of(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_of(other.c_str(), self_idx, other.size());
		}
#endif
	private:
		std::size_t _find_first_not_of(const charT* other, std::size_t self_idx, std::size_t other_count) const noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, size());
			for (std::size_t i = self_idx; i < size(); i++) {
				int j;
				for (j = 0; j < other_count; j++)
					break;
				if (j == other_count) return i;
			}
			return npos;
		}
	public:
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t find_first_not_of(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_not_of(other.c_str(), self_idx, other.size());
		}
		std::size_t find_first_not_of(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _find_first_not_of(other, self_idx, count);
		}
		std::size_t find_first_not_of(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_not_of(other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t find_first_not_of(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_not_of(&ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t find_first_not_of(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _find_first_not_of(view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t find_first_not_of(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_not_of(other.c_str(), self_idx, other.size());
		}
#endif
	private:
		std::size_t _find_last_of(const charT* other, std::size_t self_idx, std::size_t other_count) const noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, size());
			for (std::size_t i = size(); i > self_idx;) {
				--i;
				for (int j = 0; j < other_count; j++)
					if (buffer[i] == other[j]) return i;
			}
			return npos;
		}
	public:
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t find_last_of(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_of(other.c_str(), self_idx, other.size());
		}
		std::size_t find_last_of(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _find_last_of(other, self_idx, count);
		}
		std::size_t find_last_of(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_of(other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t find_last_of(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_of(&ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t find_last_of(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _find_last_of(view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t find_last_of(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_of(other.c_str(), self_idx, other.size());
		}
#endif
	private:
		std::size_t _find_last_not_of(const charT* other, std::size_t self_idx, std::size_t other_count) const noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, size());
			for (std::size_t i = size(); i > self_idx;) {
				--i;
				for (int j = 0; j < other_count; j++)
					if (buffer[i] == other[j]) return i;
			}
			return npos;
		}
	public:
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t find_last_not_of(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_not_of(other.c_str(), self_idx, other.size());
		}
		std::size_t find_last_not_of(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _find_last_not_of(other, self_idx, count);
		}
		std::size_t find_last_not_of(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_not_of(other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t find_last_not_of(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_not_of(&ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t find_last_not_of(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _find_last_not_of(view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t find_last_not_of(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_not_of(other.c_str(), self_idx, other.size());
		}
#endif
	};
	//operators
	//swap, erase, erase_if, 
	//operator<<, operator>>, getline
	//stoi, stol, stoll, stoul, stoull, stof, stod, stold, to_short_string, to_short_wstring
	//operator""ss
	//hash<short_string>, hash<short_wstring>

	template<unsigned char max_len>
	using small_string = small_basic_string<char, max_len, overflow_behavior_t::exception>;
	template<unsigned char max_len>
	using small_wstring = small_basic_string<wchar_t, max_len, overflow_behavior_t::exception>;
#if _HAS_CXX17
	template<unsigned char max_len>
	using small_u8string = small_basic_string<std::char8_t, max_len, overflow_behavior_t::exception>;
	template<unsigned char max_len>
	using small_u16string = small_basic_string<std::char16_t, max_len, overflow_behavior_t::exception>;
	template<unsigned char max_len>
	using small_u32string = small_basic_string<std::char32_t, max_len, overflow_behavior_t::exception>;
#endif
}