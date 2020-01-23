#pragma once
#include <cassert>
#include <cstddef>
#include <bitset>
#include <memory>
#include <string>
#include <vector>
#include <intrin.h>
#pragma intrinsic(_BitScanForward, _BitScanForward64)

namespace mpd {
	template<unsigned char alloc_size_bytes, unsigned char alloc_count=1>
	class allocation_buffer {
		using block = char[alloc_size_bytes];

		std::bitset<alloc_count> used;
		block buffer[alloc_count];
		int first_unused_index() {
			const int ulz = sizeof(unsigned long);
			const int ullz = sizeof(unsigned long long);
			if (alloc_count == 1) {
				return used.any() ? throw std::bad_alloc() : 0;
			}
			else if (alloc_count <= ulz) {
				unsigned long bits = used.to_ulong();
				if (bits == 0) return 0;
				if (~bits == 0) throw std::bad_alloc();
				unsigned long result;
				_BitScanForward(&result, ~bits);
				if (result >= alloc_count) throw std::bad_alloc();
				return result;
			}
			int ullong_alloc_count = (alloc_count + ullz - 1) / ullz;
			int offset = 0;
			for (int i = 0; i < ullong_alloc_count; i++) {
				unsigned long long bits = (used >> (i*ullz)).to_ullong();
				if (bits == 0) return 0;
				if (~bits == 0) {
					offset += ullz;
					continue;
				}
				unsigned long result;
				_BitScanForward64(&result, ~bits);
				if (result + offset < alloc_count) return result;
			}
			throw std::bad_alloc();
		}
	public:
		allocation_buffer() noexcept {}
		~allocation_buffer() {
			assert(!used.any());
		}
		char* raw_allocate(std::size_t size_bytes) {
			assert(size_bytes <= alloc_size_bytes);
			int idx = first_unused_index();
			used.set(idx);;
			return &(buffer[idx][0]);
		}
		void raw_deallocate(char* ptr) noexcept {
			assert((ptr - (char*)std::begin(buffer)) % sizeof(block) == 0);
			assert(ptr >= (char*)std::begin(buffer));
			assert(ptr < (char*)std::end(buffer));
			std::ptrdiff_t idx = (block*)ptr - std::begin(buffer);
			assert(idx >= 0);
			assert(idx < alloc_count);
			used.reset(idx);
		}
	};

	// One pointer, allocates from the referenced allocation_buffer
	template<class T, unsigned short alloc_size_bytes, unsigned char alloc_count>
	class buffer_allocator {
		allocation_buffer<alloc_size_bytes, alloc_count>* buffer;
	public:
		using pointer = T * ;
		using const_pointer = const T*;
		using void_pointer = void*;
		using const_void_pointer = const void*;
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		const bool propagate_on_container_copy_assignment = true;
		const bool propagate_on_container_move_assignment = true;
		const bool propagate_on_container_swap = true;
		const bool is_always_equal = false;
		template <class U> struct rebind { using other = buffer_allocator<U, alloc_size_bytes, alloc_count>; };

		buffer_allocator(allocation_buffer<alloc_size_bytes, alloc_count>& _buffer) noexcept : buffer(&_buffer) {}
		buffer_allocator(const buffer_allocator& rhs) noexcept : buffer(rhs.buffer) {}
		buffer_allocator(buffer_allocator&& rhs) noexcept : buffer(rhs.buffer) {}
		buffer_allocator& operator=(const buffer_allocator& rhs) noexcept { buffer = rhs.buffer; return *this; }
		buffer_allocator& operator=(buffer_allocator&& rhs) noexcept { buffer = rhs.buffer; return *this; }
		buffer_allocator select_on_container_copy_construction() noexcept { return buffer_allocator(*this); }

		pointer allocate(std::size_t bytes) { return (pointer)(buffer->raw_allocate(bytes)); }
		pointer allocate(std::size_t bytes, const_void_pointer hint) noexcept { return allocate(bytes); }
		void deallocate(pointer ptr, std::size_t) { buffer->raw_deallocate((char*)ptr); }
		std::size_t max_size() noexcept { return alloc_size_bytes; }
		template<class T, class...Us>
		T construct(T* ptr, Us... vs) { return new(ptr)T(std::forward<Us...>(vs)); }
		template<class T>
		void destroy(T* ptr) noexcept { ptr->~T(); }

		bool operator==(const buffer_allocator& rhs) const noexcept { return buffer == rhs.buffer; }
		bool operator!=(const buffer_allocator& rhs) const noexcept { return buffer != rhs.buffer; }
	};

	// large buffer, is a allocation_buffer
	template<class T, unsigned char alloc_size_bytes, unsigned char alloc_count = 1>
	struct local_allocator 
		: allocation_buffer<alloc_size_bytes, alloc_count> {
		using pointer = T * ;
		using const_pointer = const T*;
		using void_pointer = void*;
		using const_void_pointer = const void*;
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		const bool propagate_on_container_copy_assignment = false;
		const bool propagate_on_container_move_assignment = false;
		const bool propagate_on_container_swap = false;
		const bool is_always_equal = false;
		template <class U> struct rebind { using other = local_allocator<U, alloc_size_bytes, alloc_count>; };

		local_allocator() noexcept {}
		local_allocator(const local_allocator& rhs) noexcept  {}
		local_allocator(local_allocator&& rhs) noexcept {}
		template<class U>
		local_allocator(const local_allocator<U, alloc_size_bytes, alloc_count>& rhs) noexcept {}
		local_allocator& operator=(const local_allocator& rhs) noexcept { return *this; }
		local_allocator& operator=(local_allocator&& rhs) noexcept { return *this; }
		template<class U>
		local_allocator& operator=(const local_allocator<U, alloc_size_bytes, alloc_count>& rhs) noexcept { return *this; }
		local_allocator select_on_container_copy_construction() noexcept { return local_allocator(); }

		pointer allocate(std::size_t bytes) { return (pointer)(this->raw_allocate(bytes)); }
		pointer allocate(std::size_t bytes, const_void_pointer hint) noexcept { return allocate(bytes); }
		void deallocate(pointer ptr, std::size_t) { this->raw_deallocate((char*)ptr); }
		std::size_t max_size() noexcept { return alloc_size_bytes; }
		template<class T, class...Us>
		T* construct(T* ptr, Us... vs) { return new(ptr)T(std::forward<Us...>(vs...)); }
		template<class T>
		void destroy(T* ptr) noexcept { ptr->~T(); }

		bool operator==(const local_allocator& rhs) const noexcept { return false; }
		bool operator!=(const local_allocator& rhs) const noexcept { return true; }

	};

	template<class T, unsigned char max_len>
	struct small_vector : std::vector<T, local_allocator<T, max_len*sizeof(T)>> {
		small_vector() noexcept
			{ this->reserve(max_len); }
		explicit small_vector(std::size_t count, const T& value)
		{ this->reserve(max_len); this->assign(count, value); }
		explicit small_vector(std::size_t count)
		{ this->reserve(max_len); this->resize(count); }
		template<class InputIt>
		small_vector(InputIt first, InputIt last)
			{ this->reserve(max_len); this->assign(first, last); }
		template<class alloc>
		small_vector(const std::vector < T, alloc>& other)
			{ this->reserve(max_len); this->assign(other); }
		small_vector(std::initializer_list<T> init)
			{ this->reserve(max_len); this->assign(init); }
	};

	template<class charT, unsigned char max_len>
	struct small_basic_string : std::basic_string<charT, std::char_traits<charT>, local_allocator<charT, (max_len+1*sizeof(charT))>>{
		small_basic_string() noexcept 
			{ this->reserve(max_len); }
		small_basic_string(std::size_t count, charT ch)
		{ this->reserve(max_len); this->assign(count, ch); }
		template<class alloc>
		small_basic_string(const std::basic_string<charT, std::char_traits<charT>, alloc>& other, 
			std::size_t pos, std::size_t count =std::basic_string<charT>::npos)
			{ this->reserve(max_len); this->assign(other, pos, count); }
		small_basic_string(const charT* s, std::size_t count)
			{ this->reserve(max_len); this->assign(s, count); }
		small_basic_string(const charT* s)
			{ this->reserve(max_len); this->assign(s); }
		template<class InputIt>
		small_basic_string(InputIt first, InputIt last)
			{ this->reserve(max_len); this->assign(first, last); }
		template<class alloc>
		small_basic_string(const std::basic_string<charT, std::char_traits<charT>, alloc>& other)
			{ this->reserve(max_len); this->assign(other); }
		small_basic_string(const std::initializer_list<charT>& iList) 
			{ this->reserve(max_len); this->assign(iList); }
#if _HAS_CXX17
		template<class T, std::enable_if_t<
			std::is_convertible_v<const T&, std::basic_string_view<charT>>
			&& !std::is_convertible_v<const T&, const CharT*>>=0>
		explicit small_basic_string(const T& other)
			{ this->reserve(max_len); this->assign(std::basic_string_view<charT, std::char_tarits<charT>>(other)); }
		template<class T, std::enable_if_t<
			std::is_convertible_v<const T&, std::basic_string_view<CharT, Traits>>>=0>
		small_basic_string(const T& other, std::size_t pos, std::size_t n)
			{ this->reserve(max_len); this->assign(std::basic_string_view<charT, std::char_tarits<charT>>(other, pos, n)); }
#endif
	};
	template<unsigned char max_len>
	using small_string = small_basic_string<char, max_len>;
	template<unsigned char max_len>
	using small_wstring = small_basic_string<wchar_t, max_len>;
#if _HAS_CXX17
	template<unsigned char max_len>
	using small_u8string = small_basic_string<std::char8_t, max_len>;
	template<unsigned char max_len>
	using small_u16string = small_basic_string<std::char16_t, max_len>;
	template<unsigned char max_len>
	using small_u32string = small_basic_string<std::char32_t, max_len>;
	}
#endif
}