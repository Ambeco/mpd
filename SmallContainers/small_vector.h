#pragma once
#include <cassert>
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "mpd_algorithms.h"

//inspired by https://codereview.stackexchange.com/questions/148757/short-string-class
namespace mpd {

	enum class overflow_behavior_t {
		exception,
		assert_and_truncate,
		silently_truncate // :(
	};
	struct default_not_value_construct {};
	struct do_value_construct {};
	namespace impl {
		template<overflow_behavior_t> std::size_t max_length_check(std::size_t given, std::size_t maximum);
		template<>
		inline std::size_t max_length_check<overflow_behavior_t::exception>(std::size_t given, std::size_t maximum) {
			if (given > maximum) throw std::length_error(std::to_string(given) + " overflows small_string");
			return given;
		}
		template<>
		inline std::size_t max_length_check<overflow_behavior_t::assert_and_truncate>(std::size_t given, std::size_t maximum) noexcept {
			if (given > maximum) {
				assert(false);
				return maximum;
			}
			return given;
		}
		template<>
		inline std::size_t max_length_check<overflow_behavior_t::silently_truncate>(std::size_t given, std::size_t maximum) noexcept {
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
		//small_vector_impl exists to shrink binary code so that all small_vector use the same binary code regardless of their length
		//Also most calls pass 'const T*' iterators, again to shrink the binary code. 
		template<class T, overflow_behavior_t overflow_behavior>
		struct small_vector_impl {
			static const bool overflow_throws = overflow_behavior != overflow_behavior_t::exception;
			static std::size_t max_length_check(std::size_t given, std::size_t maximum) noexcept(overflow_throws)
			{
				return impl::max_length_check<overflow_behavior>(given, maximum);
			}
			static std::size_t index_range_check(std::size_t given, std::size_t maximum) {
				if (given > maximum) throw std::out_of_range(std::to_string(given) + " is an invalid index");
				return given;
			}
			static std::size_t clamp_max(std::size_t given, std::size_t maximum) noexcept {
				if (given > maximum) return maximum;
				return given;
			}
			static MPD_NOINLINE(void) _assign(T* buffer, std::size_t& size, std::size_t max_len, std::size_t src_count, default_not_value_construct) noexcept(overflow_throws) {
				src_count = max_length_check(src_count, max_len);
				std::fill_n(buffer, size, T{});
				uninitialized_default_construct(buffer + size, buffer + src_count);
				size = src_count;
			}
			static MPD_NOINLINE(void) _assign(T* buffer, std::size_t& size, std::size_t max_len, std::size_t src_count, do_value_construct) noexcept(overflow_throws) {
				src_count = max_length_check(src_count, max_len);
				std::fill_n(buffer, size, T{});
				uninitialized_value_construct_n(buffer + size, src_count);
				size = src_count;
			}
			static MPD_NOINLINE(void) _assign(T* buffer, std::size_t& size, std::size_t max_len, std::size_t src_count, const T& src) noexcept(overflow_throws) {
				src_count = max_length_check(src_count, max_len);
				std::fill_n(buffer, size, src);
				std::uninitialized_fill_n(buffer + size, src_count, src);
				size = src_count;
			}
			template<class ForwardIt>
			static MPD_NOINLINE(void) _assign(T* buffer, std::size_t& size, std::size_t max_len, ForwardIt src_first, ForwardIt src_last, std::forward_iterator_tag) noexcept(overflow_throws) {
				std::size_t src_count = distance_up_to_n(src_first, src_last, max_len + 1);
				src_count = max_length_check(src_count, max_len);
				T* mid = std::copy_n(src_first, size, buffer);
				std::uninitialized_copy_n(std::next(src_first, size), src_count - size, mid);
				size = src_count;
			}
			template<class InputIt>
			static MPD_NOINLINE(void) _assign(T* buffer, std::size_t& size, std::size_t max_len, InputIt src_first, InputIt src_last, std::input_iterator_tag) noexcept(overflow_throws) {
				std::size_t max_ctor_count = max_len - size;
				auto it1 = copy_up_to_n(src_first, src_last, size, buffer);
				auto it2 = uninitialized_copy_up_to_n(it1.first, src_last, max_ctor_count, it1.second);
				size = it2.second - buffer;
				if (it2.first != src_last) max_length_check(max_len + 1, max_len);
			}
			template<class... Us>
			static MPD_NOINLINE(void) _emplace(T* buffer, std::size_t& size, std::size_t max_len, std::size_t dst_idx, Us&&... src) noexcept(overflow_throws) {
				dst_idx = index_range_check(dst_idx, size);
				if (dst_idx == size) {
					new (buffer + size)T(std::forward<Us>(src)...);
					++size;
					return;
				}
				std::size_t keep_count = dst_idx;
				new (buffer + size)T(std::move(buffer[size - 1]));
				++size;
				move_backward_n(buffer + dst_idx, keep_count, buffer + dst_idx + 1);
				buffer[dst_idx] = T(std::forward<Us>(src)...);
			}
			static MPD_NOINLINE(void) _insert(T* buffer, std::size_t& size, std::size_t max_len, std::size_t dst_idx, std::size_t src_count, T src) noexcept(overflow_throws) {
				dst_idx = index_range_check(dst_idx, size);
				std::size_t insert_move_count = clamp_max(src_count, size - dst_idx);
				std::size_t insert_construct_count = max_length_check(src_count, max_len - dst_idx) - insert_move_count;
				src_count = insert_move_count + insert_construct_count;
				std::size_t keep_move_count = size - dst_idx - insert_move_count;
				std::size_t keep_construct_count = max_length_check(size + src_count, max_len) - insert_construct_count;
				uninitialized_fill_n(buffer + size, insert_construct_count, src);
				size += insert_construct_count;
				uninitialized_move_backward_n(buffer + size + insert_construct_count, keep_construct_count, buffer + dst_idx + keep_move_count);
				size += keep_construct_count;
				move_backward_n(buffer + dst_idx + keep_construct_count, keep_move_count, buffer + dst_idx + insert_construct_count + insert_move_count + keep_move_count);
				std::fill_n(buffer + dst_idx, insert_move_count, src);
			}
			template<class ForwardIt>
			static MPD_NOINLINE(void) _insert(T* buffer, std::size_t& size, std::size_t max_len, std::size_t dst_idx, ForwardIt src_first, ForwardIt src_last) noexcept(overflow_throws) {
				dst_idx = index_range_check(dst_idx, size);
				std::size_t src_count = distance_up_to_n(src_first, src_last, max_len - dst_idx + 1);
				std::size_t insert_move_count = clamp_max(src_count, size - dst_idx);
				std::size_t insert_construct_count = max_length_check(src_count, max_len - dst_idx) - insert_move_count;
				src_count = insert_move_count + insert_construct_count;
				std::size_t keep_move_count = size - dst_idx - insert_move_count;
				std::size_t keep_construct_count = max_length_check(size + src_count, max_len) - insert_construct_count;
				std::uninitialized_copy_n(src_first, insert_construct_count, buffer + size);  //WRONG ITEMS These are the first, in the wrong place!
				std::advance(src_first, insert_construct_count);
				size += insert_construct_count;
				uninitialized_move_backward_n(buffer + size, keep_construct_count, buffer + dst_idx + keep_move_count);
				size += keep_construct_count;
				move_backward_n(buffer + dst_idx + keep_construct_count, keep_move_count, buffer + dst_idx + insert_construct_count + insert_move_count + keep_move_count);
				std::copy_n(src_first, insert_move_count, buffer + dst_idx);
			}
			void _erase(T* buffer, std::size_t& size, std::size_t max_len, std::size_t idx, std::size_t count) noexcept(overflow_throws) {
				idx = index_range_check(idx, size);
				count = clamp_max(count, size - idx);
				std::size_t move_count = size - idx - count;
				move_n(buffer + idx + count, move_count, buffer + idx);
				destroy(buffer + size - count, buffer + size);
				size -= count;
			}
			static MPD_NOINLINE(void) _append(T* buffer, std::size_t& size, std::size_t max_len, std::size_t src_count, do_value_construct) noexcept(overflow_throws) {
				src_count = max_length_check(src_count, max_len - size);
				uninitialized_value_construct_n(buffer + size, src_count);
				size += src_count;
			}
			static MPD_NOINLINE(void) _append(T* buffer, std::size_t& size, std::size_t max_len, std::size_t src_count, T src) noexcept(overflow_throws) {
				src_count = max_length_check(src_count, max_len - size);
				std::uninitialized_fill_n(buffer + size, src_count, src);
				size += src_count;
			}
			template<class InputIt>
			static MPD_NOINLINE(void) _append(T* buffer, std::size_t& size, std::size_t max_len, InputIt src_first, InputIt src_last, std::forward_iterator_tag) noexcept(overflow_throws) {
				std::size_t src_count = distance_up_to_n(src_first, src_last, max_len + 1);
				src_count = max_length_check(src_count, max_len - size);
				src_first = uninitialied_copy_n(src_first, src_count, buffer + size);
				size += src_count;
			}
			template<class InputIt>
			static MPD_NOINLINE(void) _append(T* buffer, std::size_t& size, std::size_t max_len, InputIt src_first, InputIt src_last, std::output_iterator_tag) noexcept(overflow_throws) {
				auto its = uninitialized_copy_up_to_n(src_first, src_last, max_len - size, buffer + size);
				size = its.second - buffer;
				if (its.first != src_last) max_length_check(max_len + 1, max_len);
			}
			static MPD_NOINLINE(void) _copy(const T* buffer, std::size_t size, T* dest, std::size_t src_count, std::size_t src_idx) noexcept {
				src_idx = index_range_check(src_idx, size);
				src_count = std::min(src_count, size - src_idx);
				dest = std::copy_n(buffer + src_idx, src_count, dest);
			}
			static MPD_NOINLINE(bool) _equals(const T* buffer, std::size_t size, const T* other, std::size_t other_size) noexcept {
				if (size != other_size) return false;
				for (std::size_t i = 0; i < size; i++) {
					if (!(buffer[i] == other[i]))
						return false;
				}
				return true;
			}
			static MPD_NOINLINE(bool) _notequals(const T* buffer, std::size_t size, const T* other, std::size_t other_size) noexcept {
				if (size != other_size) return true;
				for (std::size_t i = 0; i < size; i++) {
					if (buffer[i] != other[i])
						return true;
				}
				return false;
			}
			template<class ItemComp, class LenComp>
			static MPD_NOINLINE(bool) _compare(const T* buffer, std::size_t size, const T* other, std::size_t other_size, ItemComp op, LenComp lenComp) noexcept {
				std::size_t cmp_count = std::min(size, other_size);
				for (std::size_t i = 0; i < cmp_count; i++) {
					if (op(buffer[i], other[i]))
						return true;
					if (op(other[i], buffer[i]))
						return false;
				}
				return lenComp(size, other_size);
			}
			template<class U>
			static MPD_NOINLINE(const T*) _remove(T* buffer, std::size_t& size, const U& value) noexcept {
				return std::remove(buffer, buffer + size, value);
			}
			template<class Pred>
			static MPD_NOINLINE(const T*) _remove_if(T* buffer, std::size_t& size, Pred&& predicate) noexcept {
				return std::remove_if(buffer, buffer + size, std::forward<Pred>(predicate));
			}
			static MPD_NOINLINE(void) _swap(T* buffer, std::size_t& size, std::size_t max_len, T* other_buffer, std::size_t& other_size, std::size_t other_max) noexcept(overflow_throws) {
				if (size > other_size) {
					_swap(other_buffer, other_size, other_max, buffer, size, max_len);
					return;
				}
				std::size_t swap_count = size;
				std::size_t ctor_count = other_size - size;
				std::swap_ranges(buffer, buffer + size, other_buffer);
				uninitialized_move(other_buffer + swap_count, other_buffer + other_size, buffer + swap_count);
				size += ctor_count;
				destroy(other_buffer + swap_count, other_buffer + other_size);
				other_size -= ctor_count;
			}
		};
	}
	template<class T, std::size_t max_len, overflow_behavior_t overflow_behavior = overflow_behavior_t::exception>
	class small_vector : private impl::small_vector_impl<T, overflow_behavior> {
		using memory_t = std::aligned_storage_t<sizeof(T), alignof(T)>;
		using buffer_t = std::array<memory_t, max_len>;
		static const bool overflow_throws = overflow_behavior != overflow_behavior_t::exception;

#ifdef MPD_SVEVTOR_OVERRUN_CHECKS
		volatile T before_buffer_checker = (T)0x66;
#endif
		std::size_t len_;
		buffer_t mem_buffer;
#ifdef MPD_SVEVTOR_OVERRUN_CHECKS
		volatile T after_buffer_checker = (T)0x66;
		void check_overruns() {
			assert(before_buffer_checker == (T)0x66);
			assert(after_buffer_checker == (T)0x66);
		}
#else
		inline void check_overruns() { }
#endif

		using impl::small_vector_impl<T, overflow_behavior>::max_length_check;
		std::size_t max_length_check(std::size_t given) const noexcept(overflow_throws)
		{
			return impl::max_length_check<overflow_behavior>(given, max_len);
		}
		using impl::small_vector_impl<T, overflow_behavior>::index_range_check;
		using impl::small_vector_impl<T, overflow_behavior>::clamp_max;
	public:
		using value_type = T;
		using allocator_type = std::allocator<T>;
		using size_t = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;
		using iterator = T*;
		using const_iterator = const T*;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		constexpr small_vector() noexcept : len_(0) {}
		small_vector(std::size_t src_count) noexcept(overflow_throws)
			: len_(0)
		{
			assign(src_count);
		}
		small_vector(std::size_t src_count, default_not_value_construct def_ctor) noexcept(overflow_throws) 
			: len_(0) 
		{
			this->_assign(data(), len_, max_len, src_count, def_ctor); return *this;
		}
		small_vector(std::size_t src_count, const T& src) noexcept(overflow_throws) 
			: len_(0) 
		{
			assign(src_count, src);
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		constexpr small_vector(const small_vector<T, other_max, other_overflow>& src) noexcept(overflow_throws)
			: len_(0) 
		{
			assign(src.begin(), src.end());
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		constexpr small_vector(small_vector<T, other_max, other_overflow>&& src) noexcept(overflow_throws)
			: len_(0)
		{
			assign(std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
		}
		template<class InputIt>
		small_vector(InputIt src_first, InputIt src_last) noexcept(overflow_throws)
			: len_(0)
		{
			assign(src_first, src_last);
		}
		constexpr small_vector(const small_vector& src) noexcept
			: len_(0)
		{
			assign(src.begin(), src.end());
		}
		constexpr small_vector(small_vector&& src) noexcept
			: len_(0)
		{
			assign(std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
		}
		constexpr small_vector(std::initializer_list<T> src) noexcept(overflow_throws)
			: len_(0)
		{
			assign(src);
		}
		template<class alloc>
		explicit small_vector(const std::vector<T, alloc>& src) noexcept(overflow_throws)
			: len_(0)
		{
			assign(src.data(), src.data() + src.size());
		}
		template<class alloc>
		explicit small_vector(std::vector<T, alloc>&& src) noexcept(overflow_throws)
			: len_(0)
		{
			assign(std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
		}
		~small_vector() {
			clear();
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_vector& operator=(const small_vector<T, other_max, other_overflow>& src) noexcept(overflow_throws) 
		{ return assign(src.begin(), src.end()); }
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		small_vector& operator=(small_vector<T, other_max, other_overflow>&& src) noexcept(overflow_throws) 
		{ return assign(std::make_move_iterator(src.begin()), std::make_move_iterator(src.end())); }
		small_vector& operator=(const small_vector& src) noexcept 
		{ return assign(src.cbegin(), src.cend()); }
		small_vector& operator=(small_vector&& src) noexcept 
		{ return assign(std::make_move_iterator(src.begin()), std::make_move_iterator(src.end())); }
		small_vector& operator=(std::initializer_list<T> src) noexcept(overflow_throws) 
		{ return assign(src); }
		template<class alloc>
		small_vector& operator=(const std::vector<T, alloc>& src)
		{ return assign(src.cbegin(), src.cend()); }
		template<class alloc>
		small_vector& operator=(std::vector<T, alloc>&& src)
		{ return assign(std::make_move_iterator(src.begin()), std::make_move_iterator(src.end())); }
		small_vector& assign(std::size_t src_count) noexcept(overflow_throws) {
			this->_assign(data(), len_, max_len, src_count, do_value_construct{}); return *this;
		}
		small_vector& assign(std::size_t src_count, const T& src) noexcept(overflow_throws) {
			this->_assign(data(), len_, max_len, src_count, src); return *this;
		}
		template<class InputIt>
		small_vector& assign(InputIt src_first, InputIt src_last) noexcept(overflow_throws) {
			this->_assign(data(), len_, max_len, src_first, src_last, typename std::iterator_traits<InputIt>::iterator_category{}); 
			return *this;
		}
		small_vector& assign(std::initializer_list<T> src) noexcept(overflow_throws) {
			return assign(src.begin(), src.end());
		}
		allocator_type get_allocator() const noexcept { return {}; }
		reference at(std::size_t src_idx) { return data()[index_range_check(src_idx, len_)]; }
		const_reference at(std::size_t src_idx) const { return data()[index_range_check(src_idx, len_)]; }
		reference operator[](std::size_t src_idx) noexcept { assert(src_idx < len_); return data()[src_idx]; }
		const_reference operator[](std::size_t src_idx) const noexcept { assert(src_idx < len_); return data()[src_idx]; }
		reference front() noexcept { assert(len_ > 0); return data()[0]; }
		const_reference front() const noexcept { assert(len_ > 0); return data()[0]; }
		reference back() noexcept { assert(len_ > 0); return data()[len_ - 1]; }
		const_reference back() const noexcept { assert(len_ > 0); return data()[len_ - 1]; }
		pointer data() noexcept { return reinterpret_cast<pointer>(mem_buffer.data()); }
		const_pointer data() const noexcept { return reinterpret_cast<const_pointer >(mem_buffer.data()); }
		const_pointer cdata() const noexcept { return reinterpret_cast<const_pointer>(mem_buffer.data()); }
		explicit operator std::vector<T, std::allocator<T>>() const { return { data(), data() + len_ }; }

		iterator begin() noexcept { return data(); }
		const_iterator begin() const noexcept { return data(); }
		const_iterator cbegin() const noexcept { return data(); }
		iterator end() noexcept { return data() + len_; }
		const_iterator end() const noexcept { return data() + len_; }
		const_iterator cend() const noexcept { return data() + len_; }
		reverse_iterator rbegin() noexcept { return std::make_reverse_iterator(end()); }
		const_reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator(end()); }
		const_reverse_iterator crbegin() const noexcept { return std::make_reverse_iterator(cend()); }
		reverse_iterator rend() noexcept { return std::make_reverse_iterator(data()); }
		const_reverse_iterator rend() const noexcept { return std::make_reverse_iterator(begin()); }
		const_reverse_iterator crend() const noexcept { return std::make_reverse_iterator(cbegin()); }

		bool empty() const noexcept { return len_ == 0; }
		std::size_t size() const noexcept { return len_; }
		void reserve(std::size_t count) const noexcept(overflow_throws) { max_length_check(count); }
		std::size_t capacity() const noexcept { return max_len; }
		void shrink_to_fit() noexcept {}

		void clear() noexcept { this->_erase(data(), len_, max_len, 0, len_); }

	private:
		template<class ForwardIt>
		void _insert2(std::size_t dst_idx, ForwardIt src_first, ForwardIt src_last, std::forward_iterator_tag) noexcept(overflow_throws) {
			this->_insert(data(), len_, max_len, dst_idx, src_first, src_last);
		}
		template<class InputIt>
		void _insert2(std::size_t dst_idx, InputIt src_first, InputIt src_last, std::input_iterator_tag) noexcept(overflow_throws) {
			small_vector temp(src_first, src_last);
			this->_insert(data(), len_, max_len, dst_idx, std::make_move_iterator(temp.begin()), std::make_move_iterator(temp.end()));
		}
	public:
		iterator insert(const_iterator dst, const T& src) noexcept(overflow_throws) {
			this->_emplace(data(), len_, max_len, dst - begin(), src);
			return (iterator)dst;
		}
		iterator insert(const_iterator dst, T&& src) noexcept(overflow_throws) {
			this->_emplace(data(), len_, max_len, dst - begin(), std::move(src));
			return (iterator)dst;
		}
		iterator insert(const_iterator dst, std::size_t src_count, const T& src) noexcept(overflow_throws) {
			this->_insert(data(), len_, max_len, dst - begin(), src_count, src);
			return (iterator)dst;
		}
		template<class InputIt, class category=typename std::iterator_traits<InputIt>::iterator_category>
		iterator insert(const_iterator dst, InputIt src_first, InputIt src_last) noexcept(overflow_throws) {
			std::size_t dst_idx = dst - begin();
			_insert2(dst - begin(), src_first, src_last, typename std::iterator_traits<InputIt>::iterator_category{});
			return (iterator)dst;
		}
		iterator insert(const_iterator dst, std::initializer_list<T> src) noexcept(overflow_throws) {
			return insert(dst, src.begin(), src.end());
		}
		template<class...Us>
		iterator emplace(const_iterator dst, Us&&... src) noexcept(overflow_throws) {
			this->_emplace(data(), len_, max_len, dst - begin(), std::forward<Us>(src)...);
			return (iterator)dst;
		}

		iterator erase(const_iterator dst) noexcept(overflow_throws) {
			this->_erase(data(), len_, max_len, dst - cbegin(), 1);
			return (iterator)dst;
		}
		iterator erase(const_iterator dst, std::size_t count) noexcept(overflow_throws) {
			this->_erase(data(), len_, max_len, dst-cbegin(), count);
			return (iterator)dst;
		}
		iterator erase(const_iterator first, const_iterator last) noexcept(overflow_throws) {
			return erase(first, last - first);
		}
		void push_back(const T& src) noexcept(overflow_throws) {
			if (max_length_check(len_ + 1)) {
				new(data() + len_)T(src);
				++len_;
			}
		}
		void push_back(T&& src) noexcept(overflow_throws) {
			if (max_length_check(len_ + 1)) {
				new(data() + len_)T(std::move(src));
				++len_;
			}
		}
		template<class...Us>
		void emplace_back(Us&&... src) noexcept(overflow_throws) {
			if (max_length_check(len_ + 1)) {
				new(data() + len_)T(std::forward<Us>(src)...);
				++len_;
			}
		}
		void pop_back() noexcept {
			destroy_at(data() + len_ - 1);
			--len_;
		}

		void resize(std::size_t src_count) noexcept(overflow_throws)
		{
			src_count = max_length_check(src_count);
			if (src_count > len_)
				this->_append(data(), len_, max_len, src_count - len_, do_value_construct{});
			if (src_count < len_)
				this->_erase(data(), len_, max_len, src_count, len_ - src_count);
		}
		void resize(std::size_t src_count, T src) noexcept(overflow_throws)
		{
			src_count = max_length_check(src_count);
			if (src_count > len_)
				this->_append(data(), len_, max_len, src_count - len_, src);
			if (src_count < len_)
				this->_erase(data(), len_, max_len, src_count, len_ - src_count);
		}
		void resize(std::size_t src_count, default_not_value_construct def_ctor) noexcept(overflow_throws)
		{
			src_count = max_length_check(src_count);
			if (src_count > len_)
				this->_append(data(), len_, max_len, src_count - len_, def_ctor);
			if (src_count < len_)
				this->_erase(data(), len_, max_len, src_count, len_ - src_count);
		}
		template<std::size_t other_max, overflow_behavior_t other_overflow>
		void swap(small_vector<T, other_max, other_overflow>& other) noexcept {
			this->_swap(data(), len_, max_len, other.data(), other.len_, other_max);
		}

	};
#define MPD_SVECTOR_ONE_TEMPLATE template<class T, std::size_t max_len, mpd::overflow_behavior_t behavior>
#define MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE template<class T, std::size_t max_len, mpd::overflow_behavior_t behavior, class U, class alloc>
#define MPD_SVECTOR_TWO_TEMPLATE template<class T, std::size_t max_len, std::size_t other_len, mpd::overflow_behavior_t behavior, mpd::overflow_behavior_t other_behavior>

	//operators (short_string)
	MPD_SVECTOR_TWO_TEMPLATE
		bool operator==(const small_vector<T, max_len, behavior>& lhs, const small_vector<T, other_len, other_behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_equals(lhs.data(), lhs.size(), rhs.data(), rhs.size());
	}
	MPD_SVECTOR_TWO_TEMPLATE
		bool operator!=(const small_vector<T, max_len, behavior>& lhs, const small_vector<T, other_len, other_behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_notequals(lhs.data(), lhs.size(), rhs.data(), rhs.size());
	}
	MPD_SVECTOR_TWO_TEMPLATE
		bool operator<(const small_vector<T, max_len, behavior>& lhs, const small_vector<T, other_len, other_behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::less<T>{}, std::less<std::size_t>{});
	}
	MPD_SVECTOR_TWO_TEMPLATE
		bool operator<=(const small_vector<T, max_len, behavior>& lhs, const small_vector<T, other_len, other_behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::less<T>{}, std::less_equal<std::size_t>{});
	}
	MPD_SVECTOR_TWO_TEMPLATE
		bool operator>(const small_vector<T, max_len, behavior>& lhs, const small_vector<T, other_len, other_behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::greater<T>{}, std::greater<std::size_t>{});
	}
	MPD_SVECTOR_TWO_TEMPLATE
		bool operator>=(const small_vector<T, max_len, behavior>& lhs, const small_vector<T, other_len, other_behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::greater<T>{}, std::greater_equal<std::size_t>{});
	}
	//operators (std::vector)
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator==(const small_vector<T, max_len, behavior>& lhs, const std::vector<U, alloc>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_equals(lhs.data(), lhs.size(), rhs.data(), rhs.size());
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator==(const std::vector<U, alloc>& lhs, const small_vector<T, max_len, behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_equals(lhs.data(), lhs.size(), rhs.data(), rhs.size());
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator!=(const small_vector<T, max_len, behavior>& lhs, const std::vector<U, alloc>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_notequals(lhs.data(), lhs.size(), rhs.data(), rhs.size());
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator!=(const std::vector<U, alloc>& lhs, const small_vector<T, max_len, behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_notequals(lhs.data(), lhs.size(), rhs.data(), rhs.size());
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator<(const small_vector<T, max_len, behavior>& lhs, const std::vector<U, alloc>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::less<T>{}, std::less<std::size_t>{});
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator<(const std::vector<U, alloc>& lhs, const small_vector<T, max_len, behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::less<T>{}, std::less<std::size_t>{});
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator<=(const small_vector<T, max_len, behavior>& lhs, const std::vector<U, alloc>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::less<T>{}, std::less_equal<std::size_t>{});
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator<=(const std::vector<U, alloc>& lhs, const small_vector<T, max_len, behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::less<T>{}, std::less_equal<std::size_t>{});
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator>(const small_vector<T, max_len, behavior>& lhs, const std::vector<U, alloc>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::greater<T>{}, std::greater<std::size_t>{});
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator>(const std::vector<U, alloc>& lhs, const small_vector<T, max_len, behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::greater<T>{}, std::greater<std::size_t>{});
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator>=(const small_vector<T, max_len, behavior>& lhs, const std::vector<U, alloc>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::greater<T>{}, std::greater_equal<std::size_t>{});
	}
	MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
		bool operator>=(const std::vector<U, alloc>& lhs, const small_vector<T, max_len, behavior>& rhs) noexcept {
		return impl::small_vector_impl<T, behavior>::_compare(lhs.data(), lhs.size(), rhs.data(), rhs.size(), std::greater<T>{}, std::greater_equal<std::size_t>{});
	}
	template<class T, std::size_t max_len, overflow_behavior_t behavior, class U>
	void erase(small_vector<T, max_len, behavior>& str, const U& value) noexcept {
		auto it = std::remove_if(str.begin(), str.end(), [&](const T& v) {return v == value; });
		str.erase(it, str.end());
	}
	template<class T, std::size_t max_len, overflow_behavior_t behavior, class Pred>
	void erase_if(small_vector<T, max_len, behavior>& str, Pred&& predicate) noexcept {
		auto it = std::remove_if(str.begin(), str.end(), predicate);
		str.erase(it, str.end());
	}
}
namespace std {
	MPD_SVECTOR_TWO_TEMPLATE
	void swap(mpd::small_vector<T, max_len, behavior>& lhs, mpd::small_vector<T, other_len, other_behavior>& rhs) noexcept {
		lhs.swap(rhs);
	}
	MPD_SVECTOR_ONE_TEMPLATE
	struct hash<mpd::small_vector<T, max_len, behavior>> {
		std::size_t operator()(const mpd::small_vector<T, max_len, behavior>& val) const noexcept {
			std::hash<T> subhasher;
			std::size_t ret = 0x9e3779b9;
			for (int i = 0; i < val.size(); i++)
				ret ^= subhasher(val[i]) + 0x9e3779b9 + (ret << 6) + (ret >> 2);
			return ret;
		}
	};
}

#undef MPD_NOINLINE
#undef MPD_SVECTOR_ONE_TEMPLATE
#undef MPD_SVECTOR_ONE_AND_STDVECTOR_TEMPLATE
#undef MPD_SVECTOR_TWO_TEMPLATE