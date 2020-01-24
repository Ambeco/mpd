#pragma once
#include <cassert>
#include <climits>
#include <cstddef>
#include <algorithm>
#include <array>
#include <iostream>
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
		return { src_first, dest_first };
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

#ifdef _MSC_VER
#define MPD_NOINLINE(T) __declspec(noinline) T
#else
#define MPD_NOINLINE(T)	T __attribute__((noinline))
#endif
	//small_basic_string_helper exists to shrink binary code so that all small_strings use the same binary code regardless of their length
	//Also most calls pass 'const charT*' iterators, again to shrink the binary code. 
	template<class charT, overflow_behavior_t overflow_behavior>
	struct small_basic_string_helper {
		static const bool overflow_throws = overflow_behavior != overflow_behavior_t::exception;
		static const std::size_t npos = std::basic_string<charT>::npos;
		static std::size_t max_length_check(std::size_t given, std::size_t maximum) noexcept(overflow_throws)
		{
			return mpd::max_length_check<overflow_behavior>(given, maximum);
		}
		static std::size_t index_range_check(std::size_t given, std::size_t maximum) {
			if (given > maximum) throw std::out_of_range(std::to_string(given) + " is an invalid index");
			return given;
		}
		static MPD_NOINLINE(std::size_t) _assign(charT* buffer, std::size_t before_size, std::size_t max_len, std::size_t src_count, charT src) noexcept(overflow_throws) {
			src_count = max_length_check(src_count, max_len);
			std::fill_n(buffer, src_count, src);
			return src_count;
		}
		template<class InputIt>
		static MPD_NOINLINE(std::size_t) _assign(charT* buffer, std::size_t before_size, std::size_t max_len, InputIt src_first, InputIt src_last, std::forward_iterator_tag) noexcept(overflow_throws) {
			std::size_t src_count = distance_up_to_n(src_first, src_last, max_len + 1);
			src_count = max_length_check(src_count, max_len);
			std::copy_n(src_first, src_count, buffer);
			return src_count;
		}
		template<class InputIt>
		static MPD_NOINLINE(std::size_t) _assign(charT* buffer, std::size_t before_size, std::size_t max_len, InputIt src_first, InputIt src_last, std::output_iterator_tag) noexcept(overflow_throws) {
			auto its = copy_up_to_n(src_first, src_last, max_len, buffer);
			if (its.first != src_last) max_length_check(max_len + 1, max_len);
			std::size_t src_count = its.second - buffer;
			return src_count;
		}
		static MPD_NOINLINE(std::size_t) _insert(charT* buffer, std::size_t before_size, std::size_t max_len, std::size_t dst_idx, std::size_t src_count, charT src) noexcept(overflow_throws) {
			src_count = max_length_check(src_count, max_len - dst_idx);
			std::size_t keep = max_length_check(before_size, max_len - dst_idx - src_count);
			copy_backward_n(buffer + dst_idx + keep, keep, buffer + dst_idx + src_count + keep);
			std::fill_n(buffer + dst_idx, src_count, src);
			return dst_idx + src_count + keep;
		}
		template<class InputIt>
		static MPD_NOINLINE(std::size_t) _insert(charT* buffer, std::size_t before_size, std::size_t max_len, std::size_t dst_idx, InputIt src_first, InputIt src_last, std::forward_iterator_tag) noexcept(overflow_throws) {
			dst_idx = index_range_check(dst_idx, before_size);
			std::size_t src_count = distance_up_to_n(src_first, src_last, max_len - dst_idx + 1);
			src_count = max_length_check(src_count, max_len - dst_idx);
			std::size_t keep_end = max_length_check(before_size - dst_idx, max_len - dst_idx - src_count);
			copy_backward_n(buffer + dst_idx + keep_end, keep_end, buffer + dst_idx + src_count + keep_end);
			std::copy_n(src_first, src_count, buffer + dst_idx);
			return dst_idx + src_count + keep_end;
		}
		template<class InputIt>
		static MPD_NOINLINE(std::size_t) _insert(charT* buffer, std::size_t before_size, std::size_t max_len, std::size_t dst_idx, InputIt src_first, InputIt src_last, std::output_iterator_tag) noexcept(overflow_throws) {
			dst_idx = index_range_check(dst_idx, before_size);
			//reverse the src_last part so we keep the src_first bytes when truncating :(
			std::reverse(buffer + dst_idx, buffer+max_len);
			auto its = copy_up_to_n(src_first, src_last, max_len - dst_idx, buffer + dst_idx);
			if (its.first != src_last) max_length_check(max_len + 1, max_len);
			std::size_t ins_end_idx = its.second - buffer;
			std::size_t src_count = ins_end_idx - dst_idx;
			std::size_t keep_end = max_length_check(before_size - dst_idx, max_len - dst_idx - src_count);
			std::size_t new_size = dst_idx + src_count + keep_end;
			std::reverse(its.second, buffer + new_size); //reverse them into the right order again
			return new_size;
		}
		std::size_t _erase(charT* buffer, std::size_t before_size, std::size_t max_len, std::size_t idx, std::size_t count = npos) noexcept(overflow_throws) {
			idx = index_range_check(idx, before_size);
			count = (count > before_size - idx ? before_size - idx : count);
			std::size_t move_count = before_size - idx - count;
			std::copy_n(buffer + idx + count, move_count, buffer + idx);
			return before_size - count;
		}
		template<class InputIt>
		static MPD_NOINLINE(std::size_t) _append(charT* buffer, std::size_t before_size, std::size_t max_len, InputIt src_first, InputIt src_last, std::forward_iterator_tag) noexcept(overflow_throws) {
			std::size_t src_count = distance_up_to_n(src_first, src_last, max_len + 1);
			src_count = max_length_check(src_count, max_len - before_size);
			std::copy_n(src_first, src_count, buffer + before_size);
			return before_size + src_count;
		}
		template<class InputIt>
		static MPD_NOINLINE(std::size_t) _append(charT* buffer, std::size_t before_size, std::size_t max_len, InputIt src_first, InputIt src_last, std::output_iterator_tag) noexcept(overflow_throws) {
			auto its = copy_up_to_n(src_first, src_last, max_len- before_size, buffer + before_size);
			if (its.first != src_last) max_length_check(max_len + 1, max_len);
			std::size_t new_size = its.second - buffer;
			return new_size;
		}
		static MPD_NOINLINE(std::size_t) _replace(charT* buffer, std::size_t before_size, std::size_t max_len, std::size_t dst_idx, std::size_t rem_count, const charT* src_first, std::size_t ins_count) noexcept(overflow_throws) {
			dst_idx = index_range_check(dst_idx, before_size);
			rem_count = index_range_check(rem_count, before_size - dst_idx);
			ins_count = max_length_check(ins_count, max_len - dst_idx);
			std::size_t keep_end = (before_size - dst_idx - ins_count);
			copy_backward_n(buffer + dst_idx, keep_end, buffer + before_size + ins_count - rem_count);
			std::copy_n(src_first, ins_count, buffer + dst_idx);
			return before_size + ins_count - rem_count;
		}
		template<class InputIt>
		static std::size_t _replace(charT* buffer, std::size_t before_size, std::size_t max_len, const charT* dst_first, const charT* dst_last, InputIt src_first, InputIt src_last, std::forward_iterator_tag) noexcept(overflow_throws)
		{
			std::size_t dst_idx = index_range_check(dst_first - buffer, max_len);
			std::size_t src_count = distance_up_to_n(src_first, src_last, before_size - dst_idx + 1);
			return _replace(dst_idx, dst_last - dst_first, src_first, src_count);
		}
		template<class InputIt>
		static MPD_NOINLINE(std::size_t) _replace(charT* buffer, std::size_t before_size, std::size_t max_len, const charT* dst_first, const charT* dst_last, InputIt src_first, InputIt src_last, std::output_iterator_tag) noexcept(overflow_throws)
		{
			std::size_t last_idx = index_range_check(dst_last - buffer, max_len);
			std::size_t dst_idx = index_range_check(dst_first - buffer, last_idx);
			std::size_t rem_count = last_idx - dst_idx;
			std::size_t end_count = before_size - last_idx;
			//reverse the src_last part so we keep the src_first bytes when truncating :(
			std::reverse(buffer + dst_idx, buffer + max_len);
			auto its = copy_up_to_n(src_first, src_last, max_len - dst_idx);
			std::size_t post_ins_index = its.second - buffer;
			std::reverse(its.second, buffer + max_len);
			if (its.first != src_last) max_length_check(max_len + 1);
			end_count = max_length_check(end_count, max_len - post_ins_index);
			return post_ins_index + end_count;
		}
		static MPD_NOINLINE(int) _compare(const charT* buffer, std::size_t before_size, std::size_t self_idx, std::size_t self_count, const charT* other, std::size_t other_count) noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, before_size);
			self_count = std::min(self_count, before_size - self_idx);
			std::size_t cmp_count = std::min(self_count, other_count);
			int r = traits_type::compare(buffer + self_idx, other, cmp_count);
			if (r) return r;
			return self_count == cmp_count ? 1 : -1;
		}
		static MPD_NOINLINE(std::size_t) _find(const charT* buffer, std::size_t before_size, const charT* other, std::size_t self_idx, std::size_t other_count) noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, before_size);
			if (other_count > before_size - self_idx) return npos;
			std::size_t max = before_size - other_count;
			while (self_idx < max) {
				if (traits_type::compare(buffer + self_idx, other_count) == 0)
					return self_idx;
			}
			return npos;
		}
		static MPD_NOINLINE(std::size_t) _rfind(const charT* buffer, std::size_t before_size, const charT* other, std::size_t self_idx, std::size_t other_count) noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, before_size);
			if (other_count > before_size - self_idx) return npos;
			std::size_t max = before_size - other_count;
			for (std::size_t i = max - 1; i >= self_idx; --i) {
				if (traits_type::compare(buffer + i, other_count) == 0)
					return i;
			}
			return npos;
		}
		static MPD_NOINLINE(std::size_t) _find_first_of(const charT* buffer, std::size_t before_size, const charT* other, std::size_t self_idx, std::size_t other_count) noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, before_size);
			for (std::size_t i = self_idx; i < before_size; i++) {
				for (int j = 0; j < other_count; j++)
					if (buffer[i] == other[j]) return i;
			}
			return npos;
		}
		static MPD_NOINLINE(std::size_t) _find_first_not_of(const charT* buffer, std::size_t before_size, const charT* other, std::size_t self_idx, std::size_t other_count) noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, before_size);
			for (std::size_t i = self_idx; i < before_size; i++) {
				int j;
				for (j = 0; j < other_count; j++)
					if (buffer[i] == other[j]) break;
				if (j == other_count) return i;
			}
			return npos;
		}
		static MPD_NOINLINE(std::size_t) _find_last_of(const charT* buffer, std::size_t before_size, const charT* other, std::size_t self_idx, std::size_t other_count) noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, before_size);
			for (std::size_t i = before_size; i > self_idx;) {
				--i;
				for (int j = 0; j < other_count; j++)
					if (buffer[i] == other[j]) return i;
			}
			return npos;
		}
		static MPD_NOINLINE(std::size_t) _find_last_not_of(const charT* buffer, std::size_t before_size, const charT* other, std::size_t self_idx, std::size_t other_count) noexcept(overflow_throws) {
			self_idx = index_range_check(self_idx, before_size);
			for (std::size_t i = before_size; i > self_idx;) {
				--i;
				for (int j = 0; j < other_count; j++)
					if (buffer[i] == other[j]) return i;
			}
			return npos;
		}
		template<class U>
		static MPD_NOINLINE(const charT*) remove(const charT* buffer, std::size_t before_size, const U& value) {
			return std::remove(buffer, buffer + before_size, value);
		}
		template<class Pred>
		static MPD_NOINLINE(const charT*) remove_if(const charT* buffer, std::size_t before_size, Pred&& predicate) {
			return std::remove_if(buffer, buffer + before_size, std::forward<Pred>(predicate));
		}
	};
	template<class charT, std::size_t max_len, overflow_behavior_t overflow_behavior=overflow_behavior_t::exception>
	class small_basic_string : private small_basic_string_helper<charT, overflow_behavior> {
		static_assert(max_len < CHAR_MAX, "small_string requires a length of less than CHAR_MAX");
		using buffer_t = std::array<charT, max_len + 1>;
		static const bool overflow_throws = overflow_behavior != overflow_behavior_t::exception;

		buffer_t buffer;

		void set_size(std::size_t len) { assert(len <= max_len);  buffer[len] = charT{}; buffer[max_len] = (charT)(max_len - len); }

		using small_basic_string_helper<charT, overflow_behavior>::max_length_check;
		std::size_t max_length_check(std::size_t given) const noexcept(overflow_throws)
		{
			return mpd::max_length_check<overflow_behavior>(given, max_len);
		}
		using small_basic_string_helper<charT, overflow_behavior>::index_range_check;
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
		static const std::size_t npos = std::basic_string<charT>::npos;

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
		small_basic_string& operator=(charT src) noexcept { return assign(1, src); }
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
		small_basic_string& assign(std::size_t src_count, charT src) noexcept(overflow_throws) {
			set_size(this->_assign(data(), size(), max_len, src_count, src)); return *this;
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& assign(const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws) {
			return assign(src.data(), src.data() + src.size());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& assign(const small_basic_string<charT, other_max, other_overflow>& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws) {
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return assign(src.data() + src_idx, src.data() + src_idx + src_count);
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
			set_size(this->_assign(data(), size(), max_len, src_first, src_last, typename std::iterator_traits<InputIt>::iterator_category{})); return *this;
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
			return assign(src.data(), src.data() + src.size());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& assign(const T& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return assign(src.data() + src_idx, src.data() + src_idx + src_count);
		}
#else 
		template<class alloc>
		small_basic_string& assign(const std::basic_string<charT, traits_type, alloc>& src, std::size_t src_idx = 0, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			 return assign(src.data() + src_idx, src.data() + src_idx + src_count);
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
		explicit operator std::basic_string<charT, traits_type, std::allocator<charT>>() const { return { buffer.c_str(), size() }; }
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
		std::size_t size() const noexcept { return max_len - (unsigned char)(buffer[max_len]); }
		std::size_t length() const noexcept { return size(); }
		void reserve(std::size_t count) const noexcept(overflow_throws) { max_length_check(count); }
		std::size_t capacity() const noexcept { return max_len; }
		void shrink_to_fit() noexcept {}

		void clear() noexcept { set_size(0); }

		small_basic_string& insert(std::size_t dst_idx, std::size_t src_count, charT src) noexcept(overflow_throws) {
			set_size(this->_insert(data(), size(), max_len, dst_idx, src_count, src)); return *this;
		}
		small_basic_string& insert(std::size_t dst_idx, const charT* src) noexcept(overflow_throws) {
			std::size_t src_count = strnlen_s_algorithm(src, max_len - dst_idx + 1);
			insert(begin() + dst_idx, src, src + src_count);
			return *this;
		}
		small_basic_string& insert(std::size_t dst_idx, const charT* src, std::size_t src_count) noexcept(overflow_throws) {
			insert(begin() + dst_idx, src, src + src_count); return *this;
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& insert(std::size_t dst_idx, const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws) {
			insert(begin() + dst_idx, src.data(), src.data() + src.size()); return *this;
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& insert(std::size_t dst_idx, const small_basic_string<charT, other_max, other_overflow>& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws) {
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			insert(begin() + dst_idx, src.data() + src_idx, src.data() + src_idx + src_count);
			return *this;
		}
		iterator insert(const_iterator dst, charT src) noexcept(overflow_throws) {
			set_size(this->_insert(data(), size(), max_len, dst - cbegin(), 1, src));
			return begin() + (dst - cbegin());
		}
		iterator insert(const_iterator dst, std::size_t src_count, charT src) noexcept(overflow_throws) {
			set_size(this->_insert(data(), size(), max_len, dst - cbegin(), src_count, src));
			return begin() + (dst - cbegin());
		}
		template<class InputIt>
		iterator insert(const_iterator dst, InputIt src_first, InputIt src_last) noexcept(overflow_throws) {
			std::size_t dst_idx = dst - begin();
			set_size(this->_insert(data(), size(), max_len, dst_idx, src_first, src_last, typename std::iterator_traits<InputIt>::iterator_category{}));
			return begin() + dst_idx;
		}
		small_basic_string& insert(std::size_t dst_idx, std::initializer_list<charT> src) noexcept(overflow_throws) {
			insert(begin() + dst_idx, src.begin(), src.end()); return *this;
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& insert(std::size_t dst_idx, const T& src) noexcept(overflow_throws)
		{
			this->_insert(data(), size(), max_len, dst_idx, src.data(), src.data() + src.size()); return *this;
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& insert(std::size_t dst_idx, const T& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			src_idx = index_range_check(src_idx, view.size());
			src_count = (src_count > view.size() - src_idx ? view.size() - src_idx : src_count);
			this->_insert(data(), size(), max_len, dst_idx, view.data() + src_idx, view.data() + src_idx + src_count);
			return *this;
		}
#else
		template<class alloc>
		small_basic_string& insert(std::size_t dst_idx, const std::basic_string<charT, traits_type, alloc>& src, std::size_t src_idx = 0, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			insert(cbegin()+dst_idx, src.data() + src_idx, src.data() + src_idx + src_count);
			return *this;
		}
#endif

		small_basic_string& erase() noexcept {
			set_size(0);
			return *this;
		}
		small_basic_string& erase(std::size_t idx, std::size_t count = npos) noexcept(overflow_throws) {
			set_size(this->_erase(data(), size(), max_len, idx, count));
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
		small_basic_string& append(std::size_t src_count, charT src) noexcept(overflow_throws) {
			src_count = max_length_check(src_count, max_len - size());
			std::fill_n(data() + size(), src_count, src);
			set_size(size() + src_count);
			return *this;
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& append(const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws) {
			return append(src.data(), src.data() + src.size());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& append(const small_basic_string<charT, other_max, other_overflow>& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws) {
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return append(src.data() + src_idx, src.data() + src_idx + src_count);
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
			set_size(this->_append(data(), size(), max_len, src_first, src_last, typename std::iterator_traits<InputIt>::iterator_category{}));
			return *this;
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
			return append(view.data(), view.data() + view.size());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& append(const T& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			src_idx = index_range_check(src_idx, view.size());
			src_count = (src_count > view.size() - src_idx ? view.size() - src_idx : src_count);
			return append(view.data() + src_idx, view.data() + src_idx + src_count);
		}
#else
		template<class alloc>
		small_basic_string& append(const std::basic_string<charT, traits_type, alloc>& src, std::size_t src_idx = 0, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return append(src.data() + src_idx, src.data() + src_idx + src_count);
		}
#endif
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& operator+=(const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws) { return append(src); }
		small_basic_string& operator+=(charT src) noexcept(overflow_throws) { return append(1, src); }
		small_basic_string& operator+=(const charT* src) noexcept(overflow_throws) { return append(src); }
		small_basic_string& operator+=(std::initializer_list<charT> src) noexcept(overflow_throws) { return append(src); }
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& operator+=(const T& src) noexcept(overflow_throws) { return append(src); }
#else
		template<class alloc>
		small_basic_string& operator+=(const std::basic_string<charT, traits_type, alloc>& src) noexcept(overflow_throws)
		{
			return append(src);
		}
#endif
	private:
		template<class InputIt>
		small_basic_string& _replace(const_iterator rem_first, const_iterator rem_last, InputIt src_first, std::size_t ins_count) noexcept(overflow_throws)
		{
			set_size(_replace(data(), size(), max_len, rem_first - cbegin(), rem_last - rem_first, src_first, ins_count, typename std::iterator_traits<InputIt>::iterator_category{})); return *this;
		}
		small_basic_string& _replace(std::size_t dst_idx, std::size_t rem_count, const charT* src_first, std::size_t ins_count) noexcept(overflow_throws)
		{
			set_size(_replace(data(), size(), max_len, dst_idx, rem_count, src_first, ins_count)); return *this;
		}
	public:
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& replace(std::size_t dst_idx, std::size_t rem_count, const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws)
		{
			return _replace(dst_idx, rem_count, src.data(), src.size());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& replace(const_iterator rem_first, const_iterator rem_last, const small_basic_string<charT, other_max, other_overflow>& src) noexcept(overflow_throws)
		{
			return _replace(rem_first, rem_last, src.data(), src.size());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_basic_string& replace(std::size_t dst_idx, std::size_t rem_count, const small_basic_string<charT, other_max, other_overflow>& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return _replace(dst_idx, rem_count, src.data() + src_idx, src_count);
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
			return _replace(rem_first, rem_last, src.data(), src.size());
		}

#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& replace(std::size_t dst_idx, std::size_t dst_count, const T& src) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _replace(dst_idx, dst_count, view.data(), view.size());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& replace(const_iterator dst_first, const_iterator dst_last, const T& src) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _replace(dst_first, dst_last, view.data(), view.size());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			small_basic_string& replace(std::size_t dst_idx, std::size_t dst_count, const T& src, std::size_t src_idx, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			src_idx = index_range_check(src_idx, view.size());
			src_count = (src_count > view.size() - src_idx ? view.size() - src_idx : src_count);
			return _replace(dst_first, dst_last, view.data() + src_idx, src_count);
		}
	}
#else 
		template<class alloc>
		small_basic_string& replace(std::size_t dst_idx, std::size_t dst_count, const std::basic_string<charT, traits_type, alloc>& src, std::size_t src_idx = 0, std::size_t src_count = npos) noexcept(overflow_throws)
		{
			src_idx = index_range_check(src_idx, src.size());
			src_count = (src_count > src.size() - src_idx ? src.size() - src_idx : src_count);
			return _replace(dst_idx, dst_count, src.data() + src_idx, src_count);
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
				std::fill(data() + sz, data() + src_count, src);
			if (src_count != sz)
				set_size(src_count);
		}
		void swap(small_basic_string& other) noexcept {
			auto self_it = buffer.data();
			auto other_it = other.buffer.data();
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
			std::copy(data() + src_idx, data() + src_idx + src_count, dest);
			return src_count;
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		int compare(const small_basic_string<charT, other_max, other_overflow>& other) const noexcept
		{
			return traits_type::compare(data(), other.data(), size() + 1);
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		int compare(std::size_t self_idx, std::size_t self_count, const small_basic_string<charT, other_max, other_overflow>& other) const noexcept(overflow_throws)
		{
			return _compare(data(), size(), self_idx, self_count, other.data(), other.size());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		int compare(std::size_t self_idx, std::size_t self_count, const small_basic_string<charT, other_max, other_overflow>& other, std::size_t other_idx, std::size_t other_count = npos) const noexcept(overflow_throws) {
			other_idx = index_range_check(other_idx, other.size());
			other_count = std::min(other_count, other.size() - other_idx);
			return _compare(data(), size(), self_idx, self_count, other.data() + other_idx, other_count);
		}
		int compare(const charT* other) const noexcept
		{
			return traits_type::compare(data(), other, size() + 1);
		}
		int compare(std::size_t self_idx, std::size_t self_count, const charT* other) const noexcept(overflow_throws)
		{
			return _compare(data(), size(), self_idx, self_count, other, strnlen_s_algorithm(other, self_count + 1));
		}
		int compare(std::size_t self_idx, std::size_t self_count, const charT* other, std::size_t other_count) const noexcept(overflow_throws)
		{
			return _compare(data(), size(), self_idx, self_count, other, other_count);
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
			return _compare(data(), size(), self_idx, self_count, view.data(), view.size());
		}
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			int compare(std::size_t self_idx, std::size_t self_count, const T& other, std::size_t other_idx, std::size_t other_count = npos) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			other_idx = index_range_check(other_idx, view.size());
			other_count = std::min(other_count, view.size() - other_idx);
			return _compare(data(), size(), self_idx, self_count, view.data() + other_idx, other_count);
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
			return _compare(data(), size(), self_idx, self_count, other.data() + other_idx, other_count);
		}
#endif
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t find(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find(data(), size(), other.c_str(), self_idx, other.size());
		}
		std::size_t find(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _find(data(), size(), other, self_idx, count);
		}
		std::size_t find(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find(data(), size(), other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t find(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find(data(), size(), &ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t find(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _find(data(), size(), view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t find(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find(data(), size(), other.c_str(), self_idx, other.size());
		}
#endif
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t rfind(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _rfind(data(), size(), other.c_str(), self_idx, other.size());
		}
		std::size_t rfind(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _rfind(data(), size(), other, self_idx, count);
		}
		std::size_t rfind(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _rfind(data(), size(), other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t rfind(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _rfind(data(), size(), &ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t rfind(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _rfind(data(), size(), view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t rfind(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _rfind(data(), size(), other.c_str(), self_idx, other.size());
		}
#endif
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t find_first_of(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_of(data(), size(), other.c_str(), self_idx, other.size());
		}
		std::size_t find_first_of(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _find_first_of(data(), size(), other, self_idx, count);
		}
		std::size_t find_first_of(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_of(data(), size(), other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t find_first_of(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_of(data(), size(), &ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t find_first_of(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _find_first_of(data(), size(), view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t find_first_of(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_of(data(), size(), other.c_str(), self_idx, other.size());
		}
#endif
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t find_first_not_of(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_not_of(data(), size(), other.c_str(), self_idx, other.size());
		}
		std::size_t find_first_not_of(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _find_first_not_of(data(), size(), other, self_idx, count);
		}
		std::size_t find_first_not_of(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_not_of(data(), size(), other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t find_first_not_of(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_not_of(data(), size(), &ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t find_first_not_of(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _find_first_not_of(data(), size(), view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t find_first_not_of(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_first_not_of(data(), size(), other.c_str(), self_idx, other.size());
		}
#endif
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t find_last_of(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_of(data(), size(), other.c_str(), self_idx, other.size());
		}
		std::size_t find_last_of(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _find_last_of(data(), size(), other, self_idx, count);
		}
		std::size_t find_last_of(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_of(data(), size(), other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t find_last_of(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_of(data(), size(), &ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t find_last_of(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _find_last_of(data(), size(), view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t find_last_of(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_of(data(), size(), other.c_str(), self_idx, other.size());
		}
#endif
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		std::size_t find_last_not_of(const small_basic_string<charT, other_max, other_overflow>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_not_of(data(), size(), other.c_str(), self_idx, other.size());
		}
		std::size_t find_last_not_of(const charT* other, std::size_t self_idx, std::size_t count) const noexcept(overflow_throws)
		{
			return _find_last_not_of(data(), size(), other, self_idx, count);
		}
		std::size_t find_last_not_of(const charT* other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_not_of(data(), size(), other, self_idx, strnlen_s_algorithm(other, size() + 1));
		}
		std::size_t find_last_not_of(charT* ch, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_not_of(data(), size(), &ch, self_idx, 1);
		}
#if _HAS_CXX17
		template<class T, std::enable_if_t <
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT* >>= 0>
			std::size_t find_last_not_of(const T& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			std::basic_string_view<charT, traits_type> view(src);
			return _find_last_not_of(data(), size(), view.c_str(), self_idx, view.size());
		}
#else
		template<class alloc>
		std::size_t find_last_not_of(const std::basic_string<charT, traits_type, alloc>& other, std::size_t self_idx = 0) const noexcept(overflow_throws)
		{
			return _find_last_not_of(data(), size(), other.c_str(), self_idx, other.size());
		}
#endif
	};
#define MPD_SSTRING_ONE_TEMPLATE template<class charT, std::size_t max_len, mpd::overflow_behavior_t behavior>
#define MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE template<class charT, std::size_t max_len, mpd::overflow_behavior_t behavior, class alloc>
#define MPD_SSTRING_TWO_TEMPLATE template<class charT, std::size_t max_len, std::size_t other_len, mpd::overflow_behavior_t behavior, mpd::overflow_behavior_t other_behavior>
	//operator+
	MPD_SSTRING_TWO_TEMPLATE
	small_basic_string<charT, max_len + other_len, behavior> operator+(const small_basic_string<charT, max_len, behavior>& lhs, const small_basic_string<charT, other_len, other_behavior>& rhs) noexcept {
		return small_basic_string<charT, max_len + other_len, behavior>(lhs).append(rhs);
	}
	MPD_SSTRING_TWO_TEMPLATE
	small_basic_string<charT, max_len, behavior> operator+(const small_basic_string<charT, max_len, behavior>& lhs, const charT* rhs) {
		return small_basic_string<charT, max_len, behavior> (lhs).append(rhs);
	}
	MPD_SSTRING_TWO_TEMPLATE
		small_basic_string<charT, max_len, behavior> operator+(const charT* lhs, const small_basic_string<charT, max_len, behavior>& rhs) {
		return small_basic_string<charT, max_len, behavior>(lhs).append(rhs);
	}
	MPD_SSTRING_TWO_TEMPLATE
		small_basic_string<charT, max_len+1, behavior> operator+(const small_basic_string<charT, max_len, behavior>& lhs, charT rhs) {
		return small_basic_string<charT, max_len+1, behavior>(lhs).append(rhs);
	}
	MPD_SSTRING_TWO_TEMPLATE
		small_basic_string<charT, max_len + 1, behavior> operator+(charT lhs, const small_basic_string<charT, max_len, behavior>& rhs) {
		return small_basic_string<charT, max_len + 1, behavior>(lhs).append(rhs);
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		small_basic_string<charT, max_len + 1, behavior> operator+(const small_basic_string<charT, max_len, behavior>& lhs, const std::basic_string<charT, std::char_traits<charT>, alloc>& rhs) {
		return small_basic_string<charT, max_len + 1, behavior>(lhs).append(rhs);
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		small_basic_string<charT, max_len + 1, behavior> operator+(const std::basic_string<charT, std::char_traits<charT>, alloc>& lhs, const small_basic_string<charT, max_len, behavior>& rhs) {
		return small_basic_string<charT, max_len + 1, behavior>(lhs).append(rhs);
	}
	//operators (short_string)
	MPD_SSTRING_TWO_TEMPLATE
	bool operator==(const small_basic_string<charT, max_len, behavior>& lhs, const small_basic_string<charT, other_len, other_behavior>& rhs) noexcept {
		return lhs.compare(rhs)==0;
	}
	MPD_SSTRING_TWO_TEMPLATE
		bool operator!=(const small_basic_string<charT, max_len, behavior>& lhs, const small_basic_string<charT, other_len, other_behavior>& rhs) noexcept {
		return lhs.compare(rhs)!=0;
	}
	MPD_SSTRING_TWO_TEMPLATE
		bool operator<(const small_basic_string<charT, max_len, behavior>& lhs, const small_basic_string<charT, other_len, other_behavior>& rhs) noexcept {
		return lhs.compare(rhs) < 0;
	}
	MPD_SSTRING_TWO_TEMPLATE
		bool operator<=(const small_basic_string<charT, max_len, behavior>& lhs, const small_basic_string<charT, other_len, other_behavior>& rhs) noexcept {
		return lhs.compare(rhs) <= 0;
	}
	MPD_SSTRING_TWO_TEMPLATE
		bool operator>(const small_basic_string<charT, max_len, behavior>& lhs, const small_basic_string<charT, other_len, other_behavior>& rhs) noexcept {
		return lhs.compare(rhs) > 0;
	}
	MPD_SSTRING_TWO_TEMPLATE
		bool operator>=(const small_basic_string<charT, max_len, behavior>& lhs, const small_basic_string<charT, other_len, other_behavior>& rhs) noexcept {
		return lhs.compare(rhs) >= 0;
	}
	//operators (charT*)
	MPD_SSTRING_ONE_TEMPLATE
		bool operator==(const small_basic_string<charT, max_len, behavior>& lhs, const charT* rhs) noexcept {
		return lhs.compare(rhs) == 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator==(const charT* lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return rhs.compare(lhs) == 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator!=(const small_basic_string<charT, max_len, behavior>& lhs, const charT* rhs) noexcept {
		return lhs.compare(rhs) != 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator!=(const charT* lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return rhs.compare(lhs) != 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator<(const small_basic_string<charT, max_len, behavior>& lhs, const charT* rhs) noexcept {
		return lhs.compare(rhs) < 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator<(const charT* lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return rhs.compare(lhs) >= 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator<=(const small_basic_string<charT, max_len, behavior>& lhs, const charT* rhs) noexcept {
		return lhs.compare(rhs) <= 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator<=(const charT* lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return rhs.compare(lhs) > 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator>(const small_basic_string<charT, max_len, behavior>& lhs, const charT* rhs) noexcept {
		return lhs.compare(rhs) > 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator>(const charT* lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return rhs.compare(lhs) <= 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator>=(const small_basic_string<charT, max_len, behavior>& lhs, const charT* rhs) noexcept {
		return lhs.compare(rhs) >= 0;
	}
	MPD_SSTRING_ONE_TEMPLATE
		bool operator>=(const charT* lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return rhs.compare(lhs) < 0;
	}
	//operators (std::string)
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator==(const small_basic_string<charT, max_len, behavior>& lhs, const std::basic_string<charT, std::char_traits<charT>,alloc>& rhs) noexcept {
		return lhs.compare(rhs) == 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
	bool operator==(const std::basic_string<charT, std::char_traits<charT>, alloc>& lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) == 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator!=(const small_basic_string<charT, max_len, behavior>& lhs, const std::basic_string<charT, std::char_traits<charT>, alloc>& rhs) noexcept {
		return lhs.compare(rhs) != 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator!=(const std::basic_string<charT, std::char_traits<charT>, alloc>& lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) != 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator<(const small_basic_string<charT, max_len, behavior>& lhs, const std::basic_string<charT, std::char_traits<charT>, alloc>& rhs) noexcept {
		return lhs.compare(rhs) < 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator<(const std::basic_string<charT, std::char_traits<charT>, alloc>& lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) < 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator<=(const small_basic_string<charT, max_len, behavior>& lhs, const std::basic_string<charT, std::char_traits<charT>, alloc>& rhs) noexcept {
		return lhs.compare(rhs) <= 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator<=(const std::basic_string<charT, std::char_traits<charT>, alloc>& lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) <= 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator>(const small_basic_string<charT, max_len, behavior>& lhs, const std::basic_string<charT, std::char_traits<charT>, alloc>& rhs) noexcept {
		return lhs.compare(rhs) > 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator>(const std::basic_string<charT, std::char_traits<charT>, alloc>& lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) > 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator>=(const small_basic_string<charT, max_len, behavior>& lhs, const std::basic_string<charT, std::char_traits<charT>, alloc>& rhs) noexcept {
		return lhs.compare(rhs) >= 0;
	}
	MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
		bool operator>=(const std::basic_string<charT, std::char_traits<charT>, alloc>& lhs, const small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) >= 0;
	}
	template<class charT, std::size_t max_len, overflow_behavior_t behavior, class U>
	void erase(small_basic_string<charT, max_len, behavior>& str, const U& value) noexcept {
		const charT* end = small_basic_string_helper<charT, behavior>::remove(str.data(), str.size(), value);
		str.resize(end-str.data());
	}
	template<class charT, std::size_t max_len, overflow_behavior_t behavior, class Pred>
	void erase(small_basic_string<charT, max_len, behavior>& str, Pred&& predicate) {
		const charT* end = small_basic_string_helper<charT, behavior>::remove_if(str.data(), str.size(), std::forward<Pred>(predicate));
		str.resize(end - str.data());
	}
	MPD_SSTRING_ONE_TEMPLATE
	std::basic_ostream<charT, std::char_traits<charT>>& operator<<(std::basic_ostream<charT, std::char_traits<charT>>& out, const small_basic_string<charT, max_len, behavior>& str) {
		return out << str.data();
	}
//	MPD_SSTRING_ONE_TEMPLATE
//	std::basic_istream<charT, std::char_traits<charT>>& operator>>(std::basic_istream<charT, std::char_traits<charT>>& in, const small_basic_string<charT, max_len, behavior>& str) {
//		return ? ;
//	}

	//getline
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
namespace std {
	MPD_SSTRING_ONE_TEMPLATE
	void swap(mpd::small_basic_string<charT, max_len, behavior>& lhs, mpd::small_basic_string<charT, max_len, behavior>& rhs) noexcept {
		lhs.swap(rhs);
	}
}

#undef MPD_NOINLINE
#undef MPD_SSTRING_ONE_TEMPLATE
#undef MPD_SSTRING_ONE_AND_STDSTRING_TEMPLATE
#undef MPD_SSTRING_TWO_TEMPLATE