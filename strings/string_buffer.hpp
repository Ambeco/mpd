#pragma once

#if __cplusplus >=  201703L
#include <string_view>
#endif

#include "containers/front_buffer.hpp"
#include "iterators/strlen_iterator.hpp"

namespace mpd {
	namespace impl {
		template<class char_t, char_t capacity_, std::size_t alignment_=alignof(char_t)>
		class string_buffer_array {
		public:
			static_assert(capacity_ < std::numeric_limits<char_t>::max(), "capacity must fit in char_t");
			static_assert(std::is_trivial_v<char_t>, "char_t must be trivial (default constructors, copy construct, move construct, copy assign, move assign, and destructor all default, recursively)");
			using value_type = char_t;
			using size_type = std::size_t;
		protected:
			static const bool copy_ctor_should_assign = true;
			static const bool move_ctor_should_assign = true;
			static const bool copy_assign_should_assign = true;
			static const bool move_assign_should_assign = true;
			static const bool dtor_should_destroy = false;
			static const std::size_t alignment = alignment_;
			// the aligned_capacity has room for one extra trailing null/size.
			static const std::size_t aligned_capacity_ = ((sizeof(char_t) * (capacity_ +1) + alignment_ - 1) / alignment_ * alignment_ / sizeof(char_t));
		private:
			alignas(alignment_) std::array<char_t, aligned_capacity_> buffer;
			// ensure that overaligned bytes are zeroed out, so that algorithms can read/write aligned blocks deterministically.
			void uninit_set_size(size_type s) { 
				buffer[capacity_] = static_cast<char_t>(capacity_ - s);
				std::memset(buffer.data() + capacity_, 0, sizeof(char_t) * (aligned_capacity_ - capacity_));
			}
		protected:
			// ensure that bytes between content and size are zeroed out, so that algorithms can read/write aligned blocks deterministically.
			void set_size(size_type s) noexcept {
				assume(s <= capacity_);
				std::memset(buffer.data() + s, 0, (capacity_ - s) * sizeof(char_t));
				buffer[capacity_] = static_cast<char_t>(capacity_ - s);
			}
		public:
			using bytebuffer_value_type = mpd::best_bytebuffer_type_t<char_t, aligned_capacity_, alignment_>;
			string_buffer_array() noexcept { uninit_set_size(0); }
			string_buffer_array(const string_buffer_array& rhs) noexcept { uninit_set_size(0); }
			template<char_t capacity2, std::size_t align2>
			string_buffer_array(const string_buffer_array<char_t, capacity2, align2>& rhs) noexcept { uninit_set_size(0); }
			string_buffer_array& operator=(const string_buffer_array<char_t, capacity_, alignment_>& rhs) noexcept { uninit_set_size(0); return *this; }
			template<char_t capacity2, std::size_t align2>
			string_buffer_array& operator=(const string_buffer_array<char_t, capacity2, align2>& rhs) noexcept { return *this; }
			~string_buffer_array() = default;
			char_t* data() noexcept { assume(is_aligned_array(buffer.data(), aligned_capacity_, alignment)); return buffer.data(); }
			const char_t* data() const noexcept { assume(is_aligned_array(buffer.data(), aligned_capacity_, alignment)); return buffer.data(); }
			size_type size() const noexcept { return capacity_ - buffer[capacity_]; }
			size_type capacity() const noexcept { return capacity_; }
			size_type aligned_capacity() const noexcept { return aligned_capacity_; }
			std::allocator<char_t> get_allocator() const { return {}; }
		};
	}

	template<class state, overflow_behavior_t overflow>
	class string_buffer : public basic_front_buffer<state, overflow> {
		using base_t = basic_front_buffer<state, overflow>;
		using char_t = typename state::value_type;
		static std::size_t strlen(const char_t* p) {
			const char_t* start = p;
			while (*p)
				++p;
			return p - start;
		}

	public:
		using value_type = typename state::value_type;
		using size_type = typename state::size_type;
		static const size_type npos = static_cast<size_type>(-1);
		using difference_type = std::ptrdiff_t;
		using reference = char_t&;
		using const_reference = const char_t&;
		using pointer = char_t*;
		using const_pointer = const char_t*;
		using iterator = char_t*;
		using const_iterator = const char_t*;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using basic_string = std::basic_string<char_t>;
#if __cplusplus >=  201703L
		using basic_string_view = std::basic_string_view<char_t>;
#endif
		using buffer_reference = string_buffer<impl::front_buffer_reference_state<value_type, size_type, state::alignment>, overflow>;

		string_buffer(const string_buffer& rhs) noexcept(noexcept(base_t(rhs))) :base_t(rhs) {}
		string_buffer(string_buffer&& rhs) noexcept(noexcept(base_t(std::move(rhs)))) :base_t(std::move(rhs)) {}
		using base_t::base_t;
		using base_t::data;
		using base_t::size;
		using base_t::capacity;
		using base_t::insert;
		using base_t::erase;
		using base_t::begin;
		using base_t::end;
		string_buffer() {}
		template<class state2, overflow_behavior_t overflow2, bool is_convertible = std::is_convertible_v<typename state2::value_type, char_t>>
		string_buffer(const string_buffer<state2, overflow2>& other, size_type pos = 0) {
			assume(pos <= other.size());
			assign(other.data() + pos, other.data() + other.size());
		}
		template<class state2, overflow_behavior_t overflow2, bool is_convertible = std::is_convertible_v<typename state2::value_type, char_t>>
		string_buffer(const string_buffer<state2, overflow2>& other, size_type pos, size_type count) {
			assume(pos <= other.size());
			if (count == npos || pos + count >= other.size()) count = other.size() - pos;
			assume(pos + count <= other.size());
			assign(other.data() + pos, other.data() + pos + count);
		}
		string_buffer(const basic_string& other, size_type pos = 0) {
			assume(pos <= other.size());
			assign(other.data() + pos, other.data() + other.size() - pos);
		}
		string_buffer(const basic_string& other, size_type pos, size_type count) {
			assume(pos <= other.size());
			if (count == npos || pos + count >= other.size()) count = other.size() - pos;
			assume(pos + count <= other.size());
			assign(other.data() + pos, other.data() + pos + count);
		}
		string_buffer(const value_type* s, size_type count) { assign(s, s + count); }
		string_buffer(const value_type* s) { assign(s, s + strlen(s)); }
		string_buffer& operator=(const base_t& rhs) noexcept(noexcept(base_t::operator=(rhs))) { base_t::operator=(rhs); return *this; }
		string_buffer& operator=(base_t&& rhs) noexcept(noexcept(base_t::operator=(std::move(rhs)))) { base_t::operator=(std::move(rhs)); return *this; }
		string_buffer& operator=(const string_buffer& rhs) noexcept(noexcept(base_t::operator=(rhs))) { base_t::operator=(rhs); return *this; }
		string_buffer& operator=(string_buffer&& rhs) noexcept(noexcept(base_t::operator=(std::move(rhs)))) { base_t::operator=(std::move(rhs)); return *this; }
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t< std::is_convertible_v<typename state2::value_type, value_type>, string_buffer&>
			operator=(const basic_front_buffer<state2, overflow2>& rhs) noexcept(noexcept(base_t::operator=(rhs)))
		{ base_t::operator=(rhs); return *this; }
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t< std::is_convertible_v<typename state2::value_type, value_type>, string_buffer&>
			operator=(basic_front_buffer<state2, overflow2>&& rhs) noexcept(noexcept(base_t::operator=(std::move(rhs))))
		{ base_t::operator=(std::move(rhs)); return *this; }
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t< std::is_convertible_v<typename state2::value_type, value_type>, string_buffer&>
			operator=(const string_buffer<state2, overflow2>&rhs) noexcept(noexcept(base_t::operator=(rhs)))
		{ base_t::operator=(rhs); return *this; }
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t< std::is_convertible_v<typename state2::value_type, value_type>, string_buffer&>
			operator=(string_buffer<state2, overflow2> && rhs) noexcept(noexcept(base_t::operator=(std::move(rhs)))) 
		{ base_t::operator=(std::move(rhs)); return *this; }
		template<class value_type>
		std::enable_if_t< std::is_convertible_v<value_type, value_type>, string_buffer&>
			operator=(std::initializer_list<value_type> rhs) noexcept(noexcept(base_t::operator=(rhs)))
		{ base_t::operator=(rhs); return *this; }
		string_buffer& operator=(const char_t* s) { assign(s, s + strlen(s)); return *this; }
		string_buffer& operator=(char_t v) { assign(1, v); return *this; }
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			operator=(const StringViewLike& rhs) {
			assign(rhs);
			return *this;
		}
		string_buffer& operator=(std::nullptr_t) = delete;
		operator buffer_reference() noexcept { return buffer_reference(*this); }
		template<class SrcIt>
		std::enable_if_t< std::is_convertible_v<typename std::iterator_traits<SrcIt>::value_type, value_type>, string_buffer&>
			assign(SrcIt first, SrcIt last) noexcept(noexcept(base_t::assign(first, last)))
		{ base_t::assign(first, last); return *this; }
		string_buffer& assign(size_type count, const value_type& value) noexcept(noexcept(base_t::assign(count, value)))
		{ base_t::assign(count, value); return *this; }
		string_buffer& assign(std::initializer_list<value_type> iList) noexcept(noexcept(base_t::assign(iList.begin(), iList.end())))
		{ base_t::assign(iList.begin(), iList.end()); return *this; }
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			assign(const string_buffer<state2, overflow2>& other) {
			assign(other.begin(), other.end()); return *this;
		}
		string_buffer& assign(const basic_string& other) { assign(other.begin(), other.end()); return *this; }
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			assign(const string_buffer<state2, overflow2>& other, size_type pos) {
			assume(pos <= other.size()); assign(other.begin() + pos, other.end()); return *this;
		}
		string_buffer& assign(const basic_string& other, size_type pos) {
			assume(pos <= other.size()); assign(other.begin() + pos, other.end()); return *this;
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			assign(const string_buffer<state2, overflow2>& other, size_type pos, size_type count) {
			assume(pos < other.size());
			if (count == npos || pos + count >= other.size()) count = other.size() - pos;
			assume(pos + count <= other.size()); 
			assign(other.begin() + pos, other.begin() + pos + count); 
			return *this;
		}
		string_buffer& assign(const basic_string& other, size_type pos, size_type count) {
			assume(pos < other.size());
			if (count == npos || pos + count >= other.size()) count = other.size() - pos;
			assume(pos + count <= other.size()); 
			assign(other.begin() + pos, other.begin() + pos + count);
			return *this;
		}
		string_buffer& assign(string_buffer&& rhs) noexcept(noexcept(base_t::operator=(std::move(rhs)))) {
			base_t::operator=(std::move(rhs)); return *this;
		}
		string_buffer& assign(const value_type* s, size_type count) {
			assign(s, s + count); return *this;
		}
		string_buffer& assign(const value_type* s) {
			assign(s, s + strlen(s)); return *this;
		}
		value_type* c_str() { return data(); }
		const value_type* c_str() const { return data(); }
#if __cplusplus >=  201703L
		operator basic_string_view() const noexcept { return {data(), size()}; }
#endif
		size_type length() const noexcept { return size(); }
		string_buffer& insert(size_type index, size_type count, char_t ch) {
			base_t::insert(data() + index, count, ch); return *this;
		}
		string_buffer& insert(std::nullptr_t index, size_type count, char_t ch) {
			base_t::insert(data(), count, ch); return *this;
		}
		string_buffer& insert(size_type index, const char_t* s) {
			base_t::insert(data() + index, strlen_iter(s), {}); return *this;
		}
		string_buffer& insert(std::nullptr_t index, const char_t* s) {
			base_t::insert(data(), strlen_iter(s), {}); return *this;
		}
		string_buffer& insert(size_type index, const char_t* s, size_type count) {
			base_t::insert(data() + index, s, s + count); return *this;
		}
		string_buffer& insert(std::nullptr_t index, const char_t* s, size_type count) {
			base_t::insert(data(), s, s + count); return *this;
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			insert(size_type index, const string_buffer<state2, overflow2>& rhs) {
			return insert(index, rhs, 0, rhs.size());
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			insert(std::nullptr_t index, const string_buffer<state2, overflow2>& rhs) {
			return insert(static_cast<size_type>(0), rhs, 0, rhs.size());
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			insert(size_type index, const string_buffer<state2, overflow2>& rhs, size_type s_index, size_type count = npos) {
			assume(s_index <= rhs.size());
			if (count == npos) count = rhs.size() - s_index;
			assume(s_index + count <= rhs.size());
			base_t::insert(data() + index, rhs.begin() + s_index, rhs.begin() + s_index + count);
			return *this;
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			insert(std::nullptr_t index, const string_buffer<state2, overflow2>& rhs, size_type s_index, size_type count = npos) {
			return insert(static_cast<size_type>(0), rhs, s_index, count);
		}
		string_buffer& insert(size_type index, const basic_string& rhs) {
			base_t::insert(data() + index, rhs.begin(), rhs.end()); return *this;
		}
		string_buffer& insert(std::nullptr_t index, const basic_string& rhs) {
			base_t::insert(data(), rhs.begin(), rhs.end()); return *this;
		}
		string_buffer& insert(size_type index, const basic_string& rhs, size_type s_index, size_type count = npos) {
			assume(s_index <= rhs.size());
			if (count == npos) count = rhs.size() - s_index;
			assume(s_index + count <= rhs.size());
			base_t::insert(data() + index, rhs.begin() + s_index, rhs.begin() + s_index + count);
			return *this;
		}
		string_buffer& insert(std::nullptr_t index, const basic_string& rhs, size_type s_index, size_type count = npos) {
			return insert(static_cast<size_type>(0), rhs, s_index, count);
		}
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			insert(size_type pos, const StringViewLike& rhs) {
			base_t::insert(data() + pos, rhs.begin(), rhs.end()); return *this;
		}
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			insert(std::nullptr_t pos, const StringViewLike& rhs) {
			return insert(static_cast<size_type>(0), rhs);
		}
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			insert(size_type pos, const StringViewLike& rhs, size_type s_index, size_type count = npos) {
			assume(s_index <= rhs.size());
			if (count == npos) count = rhs.size() - s_index;
			assume(s_index + count <= rhs.size());
			base_t::insert(data() + s_index, rhs.begin() + s_index, rhs.begin() + s_index + count);
			return *this;
		}
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			insert(std::nullptr_t pos, const StringViewLike& rhs, size_type s_index, size_type count = npos) {
			return insert(static_cast<size_type>(0), rhs, s_index, count);
		}
		string_buffer& erase(size_type index = 0, size_type count = npos) {
			assume(index <= size());
			if (count == npos) count = size() - index;
			assume(index + count <= size());
			base_t::erase(data() + index, data() + index + count);
			return *this;
		}
		string_buffer& erase(std::nullptr_t index, size_type count = npos) {
			return erase(static_cast<size_type>(0), count);
		}
		string_buffer& append(size_type count, char_t ch) {
			base_t::insert(end(), count, ch); return *this;
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			append(const string_buffer<state2, overflow2>& rhs) {
			return append(rhs, 0, rhs.size());
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			append(const string_buffer<state2, overflow2>& rhs, size_type spos, size_type count = npos) {
			assume(spos <= rhs.size());
			if (count == npos) count = rhs.size() - spos;
			assume(spos + count <= rhs.size());
			base_t::insert(end(), rhs.begin() + spos, rhs.begin() + spos + count);
			return *this;
		}
		string_buffer& append(const basic_string& rhs) {
			insert(end(), rhs.begin(), rhs.end()); return *this;
		}
		string_buffer& append(const basic_string& rhs, size_type spos, size_type count = npos) {
			assume(spos <= rhs.size());
			if (count == npos) count = rhs.size() - spos;
			assume(spos + count <= rhs.size());
			base_t::insert(end(), rhs.begin() + spos, rhs.begin() + spos + count);
			return *this;
		}
		string_buffer& append(const char_t* s) {
			base_t::insert(end(), strlen_iter(s), {}); return *this;
		}
		string_buffer& append(const char_t* s, size_type count) {
			base_t::insert(end(), s, s + count); return *this;
		}
		template<class InputIt>
		std::enable_if_t<std::is_convertible_v<typename std::iterator_traits<InputIt>::value_type, char_t>, string_buffer&>
			append(InputIt first, InputIt last) {
			base_t::insert(end(), first, last); return *this;
		}
		string_buffer& append(std::initializer_list<char_t> ilist) {
			base_t::insert(end(), ilist.begin(), ilist.end()); return *this;
		}
#if __cplusplus >=  201703L
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			append(const StringViewLike& rhs) {
			base_t::insert(end(), rhs.begin(), rhs.end()); return *this;
		}
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			append(const StringViewLike& rhs, size_type s_index, size_type count = npos) {
			assume(s_index <= rhs.size());
			if (count == npos) count = rhs.size() - s_index;
			assume(s_index + count <= rhs.size());
			base_t::insert(end(), rhs.begin() + s_index, rhs.begin() + s_index + count);
			return *this;
		}
#endif
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			operator+=(const string_buffer<state2, overflow2>& rhs) {
			return append(rhs);
		}
		string_buffer& operator+=(const basic_string& rhs) {
			return append(rhs);
		}
		string_buffer& operator+=(char_t ch) {
			return append(1u, ch);
		}
		string_buffer& operator+=(std::initializer_list<char_t> ilist) {
			return append(ilist);
		}
#if __cplusplus >=  201703L
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			operator+=(const StringViewLike& rhs) {
			return append(rhs);
		}
#endif
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			replace(size_type pos, size_type count, const string_buffer<state2, overflow2>& rhs) {
			return replace(pos, count, rhs.begin(), rhs.end());
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			replace(const_iterator first, const_iterator last, const string_buffer<state2, overflow2>& rhs) {
			return replace(first, last, rhs.begin(), rhs.end());
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, string_buffer&>
			replace(size_type pos, size_type count, const string_buffer<state2, overflow2>& rhs, size_type pos2, size_type count2 = npos) {
			assume(pos2 < rhs.size());
			if (count2 == npos) count2 = rhs.size() - pos2;
			assume(pos2 + count2 <= rhs.size());
			return replace(pos, count, rhs.begin() + pos2, rhs.begin() + pos2 + count2);
		}
		string_buffer& replace(size_type pos, size_type count, const basic_string& rhs) {
			return replace(pos, count, rhs.begin(), rhs.end());
		}
		string_buffer& replace(size_type pos, size_type count, const basic_string& rhs, size_type pos2, size_type count2 = npos) {
			assume(pos2 < rhs.size());
			if (count2 == npos) count2 = rhs.size() - pos2;
			assume(pos2 + count2 <= rhs.size());
			return replace(pos, count, rhs.begin() + pos2, rhs.begin() + pos2 + count2);
		}
		string_buffer& replace(size_type pos, size_type count, const char_t* cstr, size_type count2) {
			return replace(pos, count, cstr, cstr + count2);
		}
		string_buffer& replace(const_iterator first, const_iterator last, const char_t* cstr, size_type count2) {
			return replace(first, last, cstr, cstr + count2);
		}
		string_buffer& replace(size_type pos, size_type count, const char_t* cstr) {
			return replace(pos, count, strlen_iter(cstr), {});
		}
		string_buffer& replace(const_iterator first, const_iterator last, const char_t* cstr) {
			return replace(first, last, strlen_iter(cstr), {});
		}
		string_buffer& replace(size_type pos, size_type count, size_type count2, char_t ch) {
			return replace(pos, count, ref_iter(ch, 0), ref_iter(ch, count2));
		}
		string_buffer& replace(const_iterator first, const_iterator last, size_type count2, char_t ch) {
			return replace(first, last, ref_iter(ch, 0), ref_iter(ch, count2));
		}
	protected:
		template<class InputIt>
		std::enable_if_t<std::is_convertible_v<typename std::iterator_traits<InputIt>::value_type, char_t>, string_buffer&>
			replace(size_type pos, size_type count, InputIt first2, InputIt last2) {
			base_t::sets(front_buffer_replace<overflow>(data(), size(), capacity(), pos, count, first2, last2));
			return *this;
		}
	public:
		template<class InputIt>
		std::enable_if_t<std::is_convertible_v<typename std::iterator_traits<InputIt>::value_type, char_t>, string_buffer&>
			replace(const_iterator first, const_iterator last, InputIt first2, InputIt last2) {
			return replace(first - data(), last - first, first2, last2);
		}
		string_buffer& replace(const_iterator first, const_iterator last, std::initializer_list<char_t> ilist) {
			return replace(first, last, ilist.begin(), ilist.end());
		}
#if __cplusplus >=  201703L
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			replace(size_type pos, size_type count, const StringViewLike& rhs) {
			return replace(pos, count, rhs.begin(), rhs.end());
		}
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			replace(const_iterator first, const_iterator last, const StringViewLike& rhs) {
			return replace(first, last, rhs.begin(), rhs.end());
		}
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			replace(size_type pos, size_type count, const StringViewLike& rhs, size_type pos2, size_type count2 = npos) {
			assume(pos2 < rhs.size());
			if (count2 == npos) count2 = rhs.size() - pos2;
			assume(pos2 + count2 <= rhs.size());
			return replace(pos, count, rhs.begin() + pos2, rhs.begin() + pos2 + count2);
		}
#endif
		size_type copy(char_t* dest, size_type count, size_type pos = 0) const {
			if (count > size() - pos) count = size() - pos;
			assume(pos <= size());
			assume(pos + count <= size());
			std::copy(data() + pos, data() + pos + count, dest);
			return static_cast<size_type>(count);
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, void>
			swap(string_buffer<state2, overflow2>& other) noexcept {
			assume(size() <= other.capacity());
			assume(other.size() <= capacity());
			size_type swap_c = size() < other.size() ? size() : other.size();
			for (size_type i = 0; i < swap_c; i++)
				std::swap(data()[i], other[i]);
			if (size() > swap_c) {
				other.append(*this, swap_c, size() - swap_c);
				this->resize(swap_c);
			} else {
				append(other, swap_c, other.size() - swap_c);
				other.resize(swap_c);
			}
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, size_type >
			find(const string_buffer<state2, overflow2>& str, size_type pos = 0) const {
			return find(str.data(), pos, str.size());
		}
		size_type find(const basic_string& str, size_type pos = 0) const {
			return find(str.data(), pos, str.size());
		}
		size_type find(const char_t* s, size_type pos, size_type count) const {
			assume(pos <= size());
			if (pos + count > size()) return npos;
			const_iterator r = std::search(begin() + pos, end(), s, s + count);
			if (r == end()) return npos;
			return static_cast<size_type>(r - data());
		}
		size_type find(const char_t* s, size_type pos = 0) const {
			return find(s, pos, strlen(s));
		}
		size_type find(char_t ch, size_type pos = 0) const {
			return find(&ch, pos, 1);
		}
#if __cplusplus >=  201703L
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			size_type>
			find(const StringViewLike& str, size_type pos = 0) const {
			return find(str.data(), pos, str.size());
		}
#endif
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, size_type >
			rfind(const string_buffer<state2, overflow2>& str, size_type pos = npos) const {
			return rfind(str.data(), pos, str.size());
		}
		size_type rfind(const basic_string& str, size_type pos = npos) const {
			return rfind(str.data(), pos, str.size());
		}
		size_type rfind(const char_t* s, size_type pos, size_type count) const {
			if (pos > size()) pos = size();
			if (pos < count) return npos;
			const_iterator r = find_end(begin(), begin() + pos+count, s, s + count);
			if (r == begin()+pos) return npos;
			return static_cast<size_type>(r - data());
		}
		size_type rfind(const char_t* s, size_type pos = npos) const {
			return rfind(s, pos, strlen(s));
		}
		size_type rfind(char_t ch, size_type pos = npos) const {
			return rfind(&ch, pos, 1);
		}
#if __cplusplus >=  201703L
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			size_type>
			rfind(const StringViewLike& str, size_type pos = npos) const {
			return rfind(str.data(), pos, str.size());
		}
#endif
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, size_type >
			find_first_of(const string_buffer<state2, overflow2>& str, size_type self_idx = 0) const noexcept {
			return find_first_of(str.data(), self_idx, str.size());
		}
		size_type find_first_of(const basic_string& str, size_type self_idx = 0) const noexcept {
			return find_first_of(str.data(), self_idx, str.size());
		}
		size_type find_first_of(const char_t* str, size_type self_idx, size_type count) const noexcept {
			assume(self_idx <= size());
			const_iterator it = mpd::find_first_of(data() + self_idx, data() + size(), str, str + count);
			if (it == end()) return npos;
			return static_cast<size_type>(it - data());
		}
		size_type find_first_of(const char_t* str, size_type self_idx = 0) const noexcept {
			return find_first_of(str, self_idx, strlen(str));
		}
		size_type find_first_of(char_t ch, size_type self_idx = 0) const noexcept {
			return find_first_of(&ch, self_idx, 1);
		}
#if __cplusplus >=  201703L
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			size_type>
			find_first_of(const StringViewLike& str, size_type pos = 0) const {
			return find_first_of(str.data(), pos, str.size());
		}
#endif
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, size_type >
			find_first_not_of(const string_buffer<state2, overflow2>& str, size_type self_idx = 0) const noexcept {
			return find_first_not_of(str.data(), self_idx, str.size());
		}
		size_type find_first_not_of(const basic_string& str, size_type self_idx = 0) const noexcept {
			return find_first_not_of(str.data(), self_idx, str.size());
		}
		size_type find_first_not_of(const char_t* str, size_type self_idx, size_type count) const noexcept {
			assume(self_idx <= size());
			const_iterator it = mpd::find_first_not_of(data() + self_idx, data() + size(), str, str + count);
			if (it == end()) return npos;
			return static_cast<size_type>(it - data());
		}
		size_type find_first_not_of(const char_t* str, size_type self_idx = 0) const noexcept {
			return find_first_not_of(str, self_idx, strlen(str));
		}
		size_type find_first_not_of(char_t ch, size_type self_idx = 0) const noexcept {
			return find_first_not_of(&ch, self_idx, 1);
		}
#if __cplusplus >=  201703L
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			size_type>
			find_first_not_of(const StringViewLike& str, size_type pos = 0) const {
			return find_first_not_of(str.data(), pos, str.size());
		}
#endif
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, size_type >
			find_last_of(const string_buffer<state2, overflow2>& str, size_type self_idx = npos) const noexcept {
			return find_last_of(str.data(), self_idx, str.size());
		}
		size_type find_last_of(const basic_string& str, size_type self_idx = npos) const noexcept {
			return find_last_of(str.data(), self_idx, str.size());
		}
		size_type find_last_of(const char_t* str, size_type self_idx, size_type count) const noexcept {
			if (self_idx > size()) self_idx = size();
			const_iterator it = mpd::find_last_of(data(), data() + self_idx, str, str + count);
			if (it == data() + self_idx) return npos;
			return it - data();
		}
		size_type find_last_of(const char_t* str, size_type self_idx = npos) const noexcept {
			return find_last_of(str, self_idx, strlen(str));
		}
		size_type find_last_of(char_t ch, size_type self_idx = npos) const noexcept {
			return find_last_of(&ch, self_idx, 1);
		}
#if __cplusplus >=  201703L
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			size_type>
			find_last_of(const StringViewLike& str, size_type pos = npos) const {
			return find_last_of(str.data(), pos, str.size());
		}
#endif
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, size_type >
			find_last_not_of(const string_buffer<state2, overflow2>& str, size_type self_idx = npos) const noexcept {
			return find_last_not_of(str.data(), self_idx, str.size());
		}
		size_type find_last_not_of(const basic_string& str, size_type self_idx = npos) const noexcept {
			return find_last_not_of(str.data(), self_idx, str.size());
		}
		size_type find_last_not_of(const char_t* str, size_type self_idx, size_type count) const noexcept {
			if (self_idx > size()) self_idx = size();
			const_iterator it = mpd::find_last_not_of(data(), data() + self_idx, str, str + count);
			if (it == data() + self_idx) return npos;
			return it - data();
		}
		size_type find_last_not_of(const char_t* str, size_type self_idx = npos) const noexcept {
			return find_last_not_of(str, self_idx, strlen(str));
		}
		size_type find_last_not_of(char_t ch, size_type self_idx = npos) const noexcept {
			return find_last_not_of(&ch, self_idx, 1);
		}
#if __cplusplus >=  201703L
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			string_buffer&>
			find_last_not_of(const StringViewLike& str, size_type pos = npos) const {
			return find_last_not_of(str.data(), pos, str.size());
		}
#endif
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, int >
			compare(const string_buffer<state2, overflow2>& str) const {
			return compare(0, size(), str.data(), str.size());
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, int >
			compare(size_type pos1, size_type count1, const string_buffer<state2, overflow2>& str) const {
			return compare(pos1, count1, str.data(), str.size());
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t<std::is_convertible_v<typename state2::value_type, char_t>, int >
			compare(size_type pos1, size_type count1, const string_buffer<state2, overflow2>& str, size_type pos2, size_type count2) const {
			if (pos2 > str.size()) throw std::out_of_range(std::to_string(pos2) + " is beyond str's size of " + std::to_string(str.size()));
			if (pos2 + count2 > str.size()) count2 = str.size() - pos2;
			return compare(pos1, count1, str.data() + pos2, count2);
		}
		int compare(const basic_string& str) const {
			return compare(0, size(), str.data(), str.size());
		}
		int compare(size_type pos1, size_type count1, const basic_string& str) const {
			return compare(pos1, count1, str.data(), str.size());
		}
		int compare(size_type pos1, size_type count1, const basic_string& str, size_type pos2, size_type count2) const {
			if (pos2 > str.size()) throw std::out_of_range(std::to_string(pos2) + " is beyond str's size of " + std::to_string(str.size()));
			if (pos2 + count2 > str.size()) count2 = static_cast<size_type>(str.size() - pos2);
			return compare(pos1, count1, str.data() + pos2, count2);
		}
		int compare(const char_t* s) const {
			return compare(0, size(), s, strlen(s));
		}
		int compare(size_type pos1, size_type count1, const char_t* s) const {
			return compare(pos1, count1, s, strlen(s));
		}
		int compare(size_type pos1, size_type count1, const char_t* s, size_type count2) const {
			if (pos1 > size()) throw std::out_of_range(std::to_string(pos1) + " is beyond str's size of " + std::to_string(count2));
			if (pos1 + count1 > size()) count1 = size() - pos1;
			return mpd::compare(data() + pos1, data() + pos1 + count1, s, s + count2);
		}
#if __cplusplus >=  201703L
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			int>
			compare(const StringViewLike& str) const {
			return compare(0, size(), str.data(), str.size());
		}
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			int>
			compare(size_type pos1, size_type count1, const StringViewLike& str) const {
			return compare(pos1, count1, str.data(), str.size());
		}
		template<class StringViewLike>
		std::enable_if_t<
			std::is_convertible_v<const StringViewLike&, basic_string_view>
			&& !std::is_convertible_v<const StringViewLike&, const char_t*>,
			int>
			compare(size_type pos1, size_type count1, const StringViewLike& str, size_type pos2, size_type count2) const {
			if (pos2 > str.size()) throw std::out_of_range();
			if (pos2 + count2 > str.size()) count2 = str.size() - pos2;
			return compare(pos1, count1, str.data() + pos2, str.data() + pos2 + count2);
		}
#endif
		// TODO: starts_with
		// TODO: ends_with
		// TODO: contains
		template<class state2=state, overflow_behavior_t overflow2 = overflow>
		string_buffer<state2, overflow2> substr(size_type pos = 0, size_type count = npos) const& {
			assume(pos <= size());
			if (count == npos) count = size() - pos;
			assume(pos + count <= size());
			return string_buffer<state2, overflow2>(data() + pos, data() + pos + count);
		}
		template<size_type capacity_, overflow_behavior_t overflow2 = overflow, std::size_t align2 = state::alignment>
		string_buffer<impl::string_buffer_array<char_t, capacity_, align2>, overflow2>
			substr(size_type pos = 0, std::integral_constant<size_type, capacity_> count = {}) const& {
			assume(pos <= size());
			size_type c = count;
			if (c == npos) c = size() - pos;
			assume(pos + count <= size());
			return {data() + pos, data() + pos + c};
		}
	};

	template<class state, overflow_behavior_t overflow, class state2, overflow_behavior_t overflow2>
	bool operator==(const string_buffer<state, overflow>& l, const string_buffer<state2, overflow2>& r) {
		return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin(), r.end());
	}
	template<class state, overflow_behavior_t overflow>
	bool operator==(const string_buffer<state, overflow>& l, const typename string_buffer<state, overflow>::basic_string& r) {
		return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin(), r.end());
	}
	template<class state, overflow_behavior_t overflow>
	bool operator==(const typename string_buffer<state, overflow>::basic_string& l, const string_buffer<state, overflow>& r) {
		return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin(), r.end());
	}
	template<class state, overflow_behavior_t overflow>
	bool operator==(const string_buffer<state, overflow>& l, const typename state::value_type* r) {
		return std::equal(l.begin(), l.end(), strlen_iter(r), {});
	}
	template<class state, overflow_behavior_t overflow>
	bool operator==(const typename state::value_type* l, const string_buffer<state, overflow>& r) {
		return std::equal(strlen_iter(l), {}, r.begin(), r.end());
	}

	template<class state, overflow_behavior_t overflow, class state2, overflow_behavior_t overflow2>
	bool operator!=(const string_buffer<state, overflow>& lhs, const string_buffer<state2, overflow2>& rhs) {
		return !operator==(lhs, rhs);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator!=(const string_buffer<state, overflow>& lhs, const typename string_buffer<state, overflow>::basic_string& rhs) {
		return !operator==(lhs, rhs);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator!=(const typename string_buffer<state, overflow>::basic_string& lhs, const string_buffer<state, overflow>& rhs) {
		return !operator==(lhs, rhs);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator!=(const string_buffer<state, overflow>& lhs, const typename state::value_type* rhs) {
		return !operator==(lhs, rhs);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator!=(const typename state::value_type* lhs, const string_buffer<state, overflow>& rhs) {
		return !operator==(lhs, rhs);
	}

	template<class state, overflow_behavior_t overflow, class state2, overflow_behavior_t overflow2>
	bool operator<(const string_buffer<state, overflow>& l, const string_buffer<state2, overflow2>& r) {
		return mpd::compare(l.begin(), l.end(), r.begin(), r.end()) < 0;
	}
	template<class state, overflow_behavior_t overflow>
	bool operator<(const string_buffer<state, overflow>& l, const typename string_buffer<state, overflow>::basic_string& r) {
		return mpd::compare(l.begin(), l.end(), r.begin(), r.end()) < 0;
	}
	template<class state, overflow_behavior_t overflow>
	bool operator<(const typename string_buffer<state, overflow>::basic_string& l, const string_buffer<state, overflow>& r) {
		return mpd::compare(l.begin(), l.end(), r.begin(), r.end()) < 0;
	}
	template<class state, overflow_behavior_t overflow>
	bool operator<(const string_buffer<state, overflow>& l, const typename state::value_type* r) {
		return mpd::compare(l.begin(), l.end(), strlen_iter(r), {}) < 0;
	}
	template<class state, overflow_behavior_t overflow>
	bool operator<(const typename state::value_type* l, const string_buffer<state, overflow>& r) {
		return mpd::compare(r.begin(), r.end(), strlen_iter(l), {}) >= 0; //note flipped to reduce template instantiations
	}

	template<class state, overflow_behavior_t overflow, class state2, overflow_behavior_t overflow2>
	bool operator<=(const string_buffer<state, overflow>& l, const string_buffer<state2, overflow2>& r) {
		return mpd::compare(l.begin(), l.end(), r.begin(), r.end()) <= 0;
	}
	template<class state, overflow_behavior_t overflow>
	bool operator<=(const string_buffer<state, overflow>& l, const typename string_buffer<state, overflow>::basic_string& r) {
		return mpd::compare(l.begin(), l.end(), r.begin(), r.end()) <= 0;
	}
	template<class state, overflow_behavior_t overflow>
	bool operator<=(const typename string_buffer<state, overflow>::basic_string& l, const string_buffer<state, overflow>& r) {
		return mpd::compare(l.begin(), l.end(), r.begin(), r.end()) <= 0;
	}
	template<class state, overflow_behavior_t overflow>
	bool operator<=(const string_buffer<state, overflow>& l, const typename state::value_type* r) {
		return mpd::compare(l.begin(), l.end(), strlen_iter(r), {}) <= 0;
	}
	template<class state, overflow_behavior_t overflow>
	bool operator<=(const typename state::value_type* l, const string_buffer<state, overflow>& r) {
		return mpd::compare(r.begin(), r.end(), strlen_iter(l), {}) < 0; //note flipped to reduce template instantiations
	}

	template<class state, overflow_behavior_t overflow, class state2, overflow_behavior_t overflow2>
	bool operator>=(const string_buffer<state, overflow>& l, const string_buffer<state2, overflow2>& r) {
		return !operator<(l, r);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator>=(const string_buffer<state, overflow>& l, const typename string_buffer<state, overflow>::basic_string& r) {
		return !operator<(l, r);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator>=(const typename string_buffer<state, overflow>::basic_string& l, const string_buffer<state, overflow>& r) {
		return !operator<(l, r);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator>=(const string_buffer<state, overflow>& l, const typename state::value_type* r) {
		return !operator<(l, r);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator>=(const typename state::value_type* l, const string_buffer<state, overflow>& r) {
		return !operator<(l, r);
	}

	template<class state, overflow_behavior_t overflow, class state2, overflow_behavior_t overflow2>
	bool operator>(const string_buffer<state, overflow>& l, const string_buffer<state2, overflow2>& r) {
		return !operator<=(l, r);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator>(const string_buffer<state, overflow>& l, const typename string_buffer<state, overflow>::basic_string& r) {
		return !operator<=(l, r);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator>(const typename string_buffer<state, overflow>::basic_string& l, const string_buffer<state, overflow>& r) {
		return !operator<=(l, r);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator>(const string_buffer<state, overflow>& l, const typename state::value_type* r) {
		return !operator<=(l, r);
	}
	template<class state, overflow_behavior_t overflow>
	bool operator>(const typename state::value_type* l, const string_buffer<state, overflow>& r) {
		return !operator<=(l, r);
	}

	template<class state, overflow_behavior_t overflow, class state2, overflow_behavior_t overflow2>
	string_buffer<state, overflow> operator+(string_buffer<state, overflow> lhs, const string_buffer<state2, overflow2>& rhs) {
		return lhs.append(rhs);
	}
	template<class state, overflow_behavior_t overflow>
	string_buffer<state, overflow> operator+(string_buffer<state, overflow> lhs, const typename string_buffer<state, overflow>::basic_string& rhs) {
		return lhs.append(rhs);
	}
	template<class state, overflow_behavior_t overflow>
	typename string_buffer<state, overflow>::basic_string operator+(typename string_buffer<state, overflow>::basic_string lhs, const string_buffer<state, overflow>& rhs) {
		return lhs.append(rhs.data(), rhs.size());
	}
	template<class state, overflow_behavior_t overflow>
	string_buffer<state, overflow> operator+(string_buffer<state, overflow> lhs, const typename state::value_type* rhs) {
		return lhs.append(rhs);
	}
	template<class state, overflow_behavior_t overflow>
	string_buffer<state, overflow> operator+(string_buffer<state, overflow> lhs, typename state::value_type rhs) {
		return lhs.append(1u, rhs);
	}
	template<class state, overflow_behavior_t overflow>
	string_buffer<state, overflow> operator+(const typename state::value_type* lhs, string_buffer<state, overflow> rhs) {
		return rhs.insert(static_cast<typename state::size_type>(0), lhs);
	}
	template<class state, overflow_behavior_t overflow>
	string_buffer<state, overflow> operator+(typename state::value_type lhs, string_buffer<state, overflow> rhs) {
		return rhs.insert(static_cast<typename state::size_type>(0), 1u, lhs);
	}
	template< class CharT, class Traits, class state, overflow_behavior_t overflow, bool same_char = std::is_same_v<CharT, typename state::value_type>>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& stream, const string_buffer<state, overflow>& str) {
		typename std::basic_ostream<CharT, Traits>::sentry s(stream);
		if (s && str.size())
			stream.write(str.data(), str.size());
		return stream;
	}
	template< class CharT, class Traits, class state, overflow_behavior_t overflow, bool same_char = std::is_same_v<CharT, typename state::value_type>>
	std::basic_istream<CharT, Traits>& operator>>(std::basic_istream<CharT, Traits>& stream, string_buffer<state, overflow>& str) {
		str.clear();
		typename std::basic_istream<CharT, Traits>::sentry s(stream);
		while (s && str.size() < str.capacity()) {
			int c = stream.peek();
			if (c == Traits::eof() || std::isspace(c)) break;
			stream.get();
			str.append(1, (CharT) c);
		}
		return stream;
	}
	template< class CharT, class Traits, class state, overflow_behavior_t overflow, bool same_char = std::is_same_v<CharT, typename state::value_type>>
	std::basic_istream<CharT, Traits>& getline(std::basic_istream<CharT, Traits>& stream, string_buffer<state, overflow>& str, CharT delim = '\n') {
		str.clear();
		typename std::basic_istream<CharT, Traits>::sentry s(stream);
		while (s && str.size() < str.capacity()) {
			int c = stream.peek();
			if (c == delim || c == Traits::eof()) break;
			stream.get();
			str.append(1, (CharT) c);
		}
		return stream;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, char>, int>
		stoi(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr, int base = 10) {
		return static_cast<int>(stol(str, pos, base));
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, wchar_t>, int>
		stoi(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr, int base = 10) {
		return static_cast<int>(stol(str, pos, base));
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, char>, long>
		stol(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr, int base = 10) {
		char* end_ptr = nullptr;
		long val = std::strtol(str.data(), &end_ptr, base);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, wchar_t>, long>
		stol(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr, int base = 10) {
		wchar_t* end_ptr = nullptr;
		long val = std::wcstol(str.data(), &end_ptr, base);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, char>, long long>
		stoll(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr, int base = 10) {
		char* end_ptr = nullptr;
		long long val = std::strtoll(str.data(), &end_ptr, base);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, wchar_t>, long long>
		stoll(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr, int base = 10) {
		wchar_t* end_ptr = nullptr;
		long long val = std::wcstoll(str.data(), &end_ptr, base);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, char>, unsigned long>
		stoul(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr, int base = 10) {
		char* end_ptr = nullptr;
		unsigned long val = std::strtoul(str.data(), &end_ptr, base);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, wchar_t>, unsigned long>
		stoul(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr, int base = 10) {
		wchar_t* end_ptr = nullptr;
		unsigned long val = std::wcstoul(str.data(), &end_ptr, base);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, char>, unsigned long long>
		stoull(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr, int base = 10) {
		char* end_ptr = nullptr;
		unsigned long val = std::strtoull(str.data(), &end_ptr, base);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, wchar_t>, unsigned long long>
		stoull(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr, int base = 10) {
		wchar_t* end_ptr = nullptr;
		unsigned long val = std::wcstoull(str.data(), &end_ptr, base);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, char>, float>
		stof(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr) {
		char* end_ptr = nullptr;
		float val = std::strtof(str.data(), &end_ptr);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, wchar_t>, float>
		stof(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr) {
		wchar_t* end_ptr = nullptr;
		float val = std::wcstof(str.data(), &end_ptr);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, char>, double>
		stod(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr) {
		char* end_ptr = nullptr;
		double val = std::strtod(str.data(), &end_ptr);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, wchar_t>, double>
		stod(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr) {
		wchar_t* end_ptr = nullptr;
		double val = std::wcstod(str.data(), &end_ptr);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, char>, long double>
		stold(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr) {
		char* end_ptr = nullptr;
		long double val = std::strtold(str.data(), &end_ptr);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}
	template<class state, overflow_behavior_t overflow>
	std::enable_if_t<std::is_same_v<typename state::value_type, wchar_t>, long double>
		stold(const string_buffer<state, overflow>& str, typename state::size_type* pos = nullptr) {
		wchar_t* end_ptr = nullptr;
		long double val = std::wcstold(str.data(), &end_ptr);
		if (pos != nullptr) *pos = end_ptr - str.data();
		return val;
	}

		// TODO: only overalign if trivial
	template<char capacity, overflow_behavior_t overflow = overflow_behavior_t::exception, std::size_t alignment=alignof(max_align_t)>
	using array_string = string_buffer<impl::string_buffer_array<char, capacity, alignment>, overflow>;
	template<wchar_t capacity, overflow_behavior_t overflow = overflow_behavior_t::exception, std::size_t alignment = alignof(max_align_t)>
	using array_wstring = string_buffer<impl::string_buffer_array<wchar_t, capacity, alignment>, overflow>;
#if __cplusplus >=  201703L
	template<char8_t capacity, overflow_behavior_t overflow = overflow_behavior_t::exception, std::size_t alignment = alignof(max_align_t)>
	using array_u8string = string_buffer<impl::string_buffer_array<char8_t, capacity, alignment>, overflow>;
	template<char16_t capacity, overflow_behavior_t overflow = overflow_behavior_t::exception, std::size_t alignment = alignof(chamax_align_tr16_t)>
	using array_u16string = string_buffer<impl::string_buffer_array<char16_t, capacity, alignment>, overflow>;
	template<char32_t capacity, overflow_behavior_t overflow = overflow_behavior_t::exception, std::size_t alignment = alignof(max_align_t)>
	using array_u32string = string_buffer<impl::string_buffer_array<char32_t, capacity, alignment>, overflow>;
#endif

	//TODO: to_string
	//TODO: to_wstring
}
namespace std {
	template<class state, mpd::overflow_behavior_t overflow>
	struct hash<mpd::string_buffer<state, overflow>> {
		std::size_t operator()(const mpd::string_buffer<state, overflow>& str) const {
			return std::hash<mpd::basic_front_buffer<state, overflow>>{}(str);
		}
	};
	//TODO: std::hash
}