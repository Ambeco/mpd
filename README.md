# MPD Library

MPDLib is a C++ library with helper utlities and data structures, extending 
the C++ standard library.  These are primarily focused on micro-optimizing code. There's a lot of 
code revolving around not allocating objects on the heap.

I've been making helper utilities for years, and they are uselessly scattered around my harddrive,
and around the internet, so I'm going to polish them and merge them all here. I'm currently maintaining
compatability with C\++14, but may bump that to C\++17 at some point in the future.

# Table of Contents
- [Libraries](#Libraries)
  - [Algorithms](#Algorithms)
    - [algorithm.h](#algorithm.hpp)
  - [Concurrency](#Concurrency)
  - [Containers](#Containers)
  - [DateTime](#DateTime)
  - [Diagnostics](#Diagnostics)
  - [InputOutput](#InputOutput)
    - [istream_lit.hpp](#istream_lit.hpp)
  - [Iterators](#Iterators)
  - [Language](#Language)
  - [Localization](#Localization)
  - [Memory](#Memory)
    - [memory.hpp](#memory.hpp)
    - [stack_allocator.hpp](#stack_allocator.hpp)
  - [Metaprogramming](#Metaprogramming)
  - [Numerics](#Numerics)
  - [Ranges](#Ranges)
  - [Regex](#Regex)
  - [Strings](#Strings)
    - [string_buffer.hpp](#string_buffer.hpp)
  - [Utilities](#Utilities)
    - [erasable.hpp](#erasable.hpp)
    - [macros.hpp](#macros.hpp)
    - [pimpl.hpp](#pimpl.hpp)

# Libraries

## Algorithms

### algorithm.hpp
	
#### C++14 forwards compatability methods
 - `size`, `find_end`

#### Obvious algorithms missing from stdlib C++14
 - `template< class InputIt, class Size, class OutputIt >`  
		`OutputIt move_n(InputIt source, Size count, OutputIt dest)`
 - `template< class InputIt, class Size, class OutputIt >  
	OutputIt move_backward_n(InputIt source, Size count, OutputIt dest)`
 - `template< class ForwardIt1, class ForwardIt2 >`  
	`ForwardIt1 find_last_of(ForwardIt1 first, ForwardIt1 last, ForwardIt2 s_first, ForwardIt2 s_last)`
 - `template< class ForwardIt1, class ForwardIt2 >`  
	`ForwardIt1 find_first_not_of(ForwardIt1 first, ForwardIt1 last, ForwardIt2 s_first, ForwardIt2 s_last)`
 - `template< class ForwardIt1, class ForwardIt2 >`  
	`ForwardIt1 find_last_not_of(ForwardIt1 first, ForwardIt1 last, ForwardIt2 s_first, ForwardIt2 s_last)`
 - `template< class InputIt1, class InputIt2, class Cmp= std::less<typename std::iterator_traits<InputIt1>::value_type> >`  
	`constexpr int compare(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Cmp comp = {})`

#### Two-Range Algorithms that check the end of both ranges
- `template <class SrcIterator, class DestIterator>`  
	`std::pair<SrcIterator, DestIterator> copy_s(SrcIterator src_first, SrcIterator src_last, DestIterator dest_first, DestIterator dest_last)`
- `template <class SrcIterator, class DestIterator>`  
	`std::pair<SrcIterator, DestIterator> move_s(SrcIterator src_first, SrcIterator src_last, DestIterator dest_first, DestIterator dest_last)`
- `template <class SrcIterator, class DestIterator>`  
	`std::pair<SrcIterator, DestIterator> move_backward_s(SrcIterator src_first, SrcIterator src_last, DestIterator dest_first, DestIterator dest_last)`
	
## Concurrency

TODO: Atomic operations on bitfields

## Containers

### front_buffer.hpp

Helper methods and a wrapper class for working with front-filled buffers.

 - `enum overflow_behavior_t { exception, assert, truncate };`  
	Controls what the algorithms should do if the buffer overflows. The first throws `std::length_error`, the second calls `assert`,
	and the third silently truncates. 

#### basic_front_buffer class

- `template<class state, overflow_behavior_t overflow>`  
	`class basic_front_buffer`  
 This is the container of a buffer. It's a drop-in replacement for `std::vector`, 
except that it uses a `state` to manage memery instead of an allocator, and the buffer is not resizable. 
It does have a reserve method for compatability, which throws a `std::length_error` if given a size bigger than the capacity, 
and does nothing otherwise. Additionally, it is trivially convertable to a `mpd::buffer_reference<T, overflow>`
- `template<class T, std::size_t capacity, overflow_behavior_t overflow = overflow_behavior_t::exception>`  
`using array_buffer = basic_front_buffer<impl::front_buffer_array_state<T, capacity>, overflow>;`  
A `basic_front_buffer` built on top of an array. This is a drop-in high-performance replacement for
`std::vector` that have a maximum capacity that is small and known at compile-time. If you have a 
`std::vector` that you know is always less than 10 elements, then all you do is replace
`std::vector<T>` with `mpd::array_buffer<T, 10>` and the code will be just a teeny-bit faster.
- `template<class T, overflow_behavior_t overflow = overflow_behavior_t::exception>`  
	`using buffer_reference = basic_front_buffer<impl::front_buffer_reference_state<T>, overflow>;`
A `basic_front_buffer` that is a mutable but non-owning reference to a `basic_front_buffer`. This is
used by code that doesn't care about which `basic_front_buffer` implementation it refers to, 
but without the overhead of virtual classes. Useful for code that works with `basic_front_buffer` but
doesn't want to depend on any buffer state type.
- `template<class T, class Allocator = std::allocator<T>, overflow_behavior_t overflow = overflow_behavior_t::exception>`  
	`using dynamic_buffer = basic_front_buffer<impl::front_buffer_vector_state<T, Allocator>, overflow>;`  
A `basic_front_buffer` that is uses a heap allocated buffer, like `std::vector`. This is _slightly_ more
lightweight than `std::vector`, but honestly, the only reason I can think of to use it would be to build a `vector` class

#### algorithms for working with front-buffers
- `template<overflow_behavior_t overflow, class T>`  
	`std::size_t front_buffer_append_value_construct(T* buffer, std::size_t size, std::size_t capacity, std::size_t count)`
- `template<overflow_behavior_t overflow, class T>`  
`std::size_t front_buffer_append_default_construct(T* buffer, std::size_t size, std::size_t capacity, std::size_t count)`
- `template<overflow_behavior_t overflow, class T, class...Args>`  
`std::size_t front_buffer_emplace_back(T* buffer, std::size_t size, std::size_t capacity, Args&&...args)`
- `template<overflow_behavior_t overflow, class T, class SourceIt>`  
	`std::size_t front_buffer_append(T* buffer, std::size_t size, std::size_t capacity, SourceIt src_first, SourceIt src_last)`
-  `template <class T>`  
	`std::size_t front_buffer_pop_back(T* buffer, std::size_t size, std::size_t erase_count) noexcept`
- `template <overflow_behavior_t overflow, class T, class ForwardIterator>`  
	`std::size_t front_buffer_insert(T* buffer, std::size_t size, std::size_t capacity, std::size_t pos, ForwardIterator src_first, ForwardIterator src_last)`
- `template <class T>`  
	`std::size_t front_buffer_erase(T* buffer, std::size_t size, std::size_t pos, std::size_t erase_count) noexcept(std::is_nothrow_move_assignable_v<T>)`
- `template<overflow_behavior_t overflow, class T, class Predicate>`  
	`std::size_t front_buffer_erase_if(T* buffer, std::size_t size, Predicate pred)`
- `template<overflow_behavior_t overflow, class T, class...Args>`  
	`std::size_t front_buffer_emplace(T* buffer, std::size_t size, std::size_t capacity, const std::size_t pos, Args&&...args)`
- `template <overflow_behavior_t overflow, class T, class SourceIterator>`
	`std::size_t front_buffer_replace(T* buffer, std::size_t size, std::size_t capacity, std::size_t pos, std::size_t replace_count, SourceIterator source_begin, SourceIterator source_end)`

## DateTime

No immediate plans

## Diagnostics

TODO: logging helpers, especially around adding context during exception unwinding

## InputOutput

### istream_lit.hpp

This header provides  helper methods for streaming in literals.
Example: `std::istream >> '(' >> x >> ", " >> y >> ')';` will require the input to be in that specific format, or else
it will fail, just like any invalid input for any primtive type. 

TODO: add support for making literals optional?


## Iterators

### iterator.hpp
Helper methods for iterating up to a maximum count.
- `template< class ForwardIt >`  
	`constexpr typename std::iterator_traits<ForwardIt>::difference_type`  
		`distance_up_to_n(ForwardIt first, ForwardIt last, typename std::iterator_traits<ForwardIt>::difference_type n)`
- `template< class ForwardIt >`  
	`constexpr ForwardIt next_up_to_n(ForwardIt first, ForwardIt last, typename std::iterator_traits<ForwardIt>::difference_type n)`
	

### reference_iterator.hpp
- `template<class T>`    
`class reference_iterator`  
An iterator constructed from a reference, and optionally, an index. Dereferencing the iterator always refers to the referenced value.
This is useful for adding an overload for an iterator pair method that instead takes a value and a count.  
`void function(const T& item, int count)`  
`{return function(ref_iter(item), ref_iter(item, count));}`
- `template<class T>`    
	`reference_iterator<T> ref_iter(const T& ref, std::size_t index = 0) noexcept`  
A helper method for constructing a `reference_iterator`.

### strlen_iterator.hpp
- `template<class T>`    
`class strlen_iterator`  
An iterator that can be constructed from a string pointer (presumably a `const char*`), but if it points at `'\0'`,
then it compares as equal to any default-constructed `strlen_iterator`. This is useful for adding an overload
for an iterator pair method that instead takes a pointer to a null-terminated string.  
`void function(const char* string)`  
`{return function(strlen_iter(string), {});}`
- `template<class T>`    
	`strlen_iterator<T> strlen_iter(const T* ptr*) noexcept`  
A helper method for constructing a `strlen_iterator`.

## Language
No immediate plans

## Localization
No immediate plans

## Memory

### memory.hpp

#### C++14 forwards compatability methods
 - `destroy`, `destroy_at`, `construct_at`, `uninitialized_value_construct`, `uninitialized_default_construct`, 
	`uninitialized_move`, `uninitialized_move_n`

#### Obvious algorithms missing from stdlib C++14
 - `template<class T>`  
	`constexpr T* value_construct_at(T* p)`
 - `template<class Iterator, class T = typename std::iterator_traits<Iterator>::value_type, class...Args>`  
	`constexpr T* indirect_construct_at(Iterator p, Args&&...args)`
 - `template<class Iterator, class T = typename std::iterator_traits<Iterator>::value_type, class...Args>`  
	`constexpr T* indirect_value_construct_at(Iterator p)`
	
#### Two-Range Algorithms that check the end of both ranges
- `template<class SourceIt, class DestIt>`  
	`std::pair<SourceIt, DestIt> uninitialized_copy_s(SourceIt src_first, SourceIt src_last, DestIt dest_first, DestIt dest_last)`
- `template<class SourceIt, class DestIt>`  
	`std::pair<SourceIt, DestIt> uninitialized_move_s(SourceIt src_first, SourceIt src_last, DestIt dest_first, DestIt dest_last)`
		
### stack_allocator.hpp

- `template<unsigned char alloc_size_bytes, unsigned char alloc_count = 1>`  
	`class allocation_buffer`  
A buffer that reserves space locally that can allocate and deallocate blocks of  `alloc_size_bytes`.
- `template<class T, unsigned short alloc_size_bytes, unsigned char alloc_count>`  
	`class buffer_allocator`  
A standard-conforming allocator constructed from a `allocation_buffer` that delegates all allocations to the buffer.
These two classes form a pair that can eliminate the heap allocationsn of standard-conforming containers,
without the risk of actually changing the container.
- `template<class T, unsigned char alloc_size_bytes, unsigned char alloc_count = 1>`  
	`struct local_allocator`  
A standard-confirming allocator that is its own buffer. This is a cleaner interface, but if the vector
is moved, then the contained elements are copied, which can be unexpectedly slow.
- `template<class T, unsigned char max_len>`  
	`class small_std_vector`  
A `std::vector<T, local_allocator>`.
- `template<class T, unsigned char max_len>`  
	`class small_std_basic_string`  
A `std::basic_string<T, local_allocator>`.
- `template<unsigned char max_len>`  
	`using small_std_string = small_std_basic_string<char, max_len>;`  
- `template<unsigned char max_len>`  
	`using small_std_wstring = small_std_basic_string<wchar_t, max_len>;`  

## metaprogramming
TODO: I'm sure I have code that goes here, but don't yet remember what

## numerics
TODO: bignum

## ranges
No immediate plans

## regex
No immediate plans

## strings

### string_buffer.hpp

- `template<class state, overflow_behavior_t overflow>`  
	`class string_buffer`  
 This is a [`mpd::front_buffer`](<#basic_front_buffer class>) that is a drop-in replacement for `std::basic_string`, 
except for how noted in the documentation for `front_buffer`. Unlike other `front_buffer` states, 
the `mpd::impl::string_buffer_array` state zeroes all unused values in the buffer, and store the length
as the number of unused characters in the last character. Ergo, a `array_string<63>` is exactly 64 bytes
long. It can contain a 63-character string, and a trailing null character.  This also means that the
length is bound by the maximum value that can be held by the char type. For `array_string`, the maximum
capacity is 127, and `array_wstring`, the maximum capacity is 32767.
- `template<char capacity, overflow_behavior_t overflow = overflow_behavior_t::exception>`  
	`using array_string = string_buffer<impl::string_buffer_array<char, capacity>, overflow>;`
- `template<wchar_t capacity, overflow_behavior_t overflow = overflow_behavior_t::exception>`  
	`using array_wstring = string_buffer<impl::string_buffer_array<wchar_t, capacity>, overflow>;`


## utilities

### erasable.hpp
`template<class Interface, std::size_t buffer_size, std::size_t align_size = alignof(std::max_align_t), bool allow_heap = false, bool noexcept_move = false, bool noexcept_copy = false>`  
	`class erasable`  
A class similar to `std::unique_pt<interface>` but stack allocated, like `std::optional`. This should be 
a drop-in replacement for `std::unique_pt<interface>`, but also has the methods of `std::optional`. This allows a
`std::vector<erasable<Animal, 32>>` to hold values of type `Dog` and `Cat` without slicing. Or better:
`mpd::front_buffer<erasable<Animal, 32>, 32>` to avoid any heap allocations at all.

### macros.hpp
#### assume
This macro is identical to `assert` in debug builds, but in `NDEBUG` builds, it triggers undefined behavior
for the condition, allowing the compiler to assume the condition is always true, and generate more optimized code.

#### MPD_NOINLINE
This is a simple way to mark a function as `noinline` that works in MSVC, G++, and Clang.

### pimpl.hpp
`template<class Impl, std::size_t buffer_size, std::size_t align_size = alignof(std::max_align_t), bool noexcept_move = false, bool noexcept_copy = true>`  
`class pimpl`  
A class similar to `std::unique_pt<Impl>` but stack allocated, like `std::optional`. This should be 
a drop-in replacement for `std::unique_pt<Impl>` in the pimpl pattern, but also has the methods of `std::optional`. This allows a
This is very similar to [`mpd::erasable`](#erasable.hpp), but more light weight, as it handle assigning 
between different possible implementation types.
```
class fooimpl;
class foo {
    mpd::pimpl<fooimpl, 32> impl;
public:
    foo(int v);
    foo(const foo& rhs);
    foo(foo&& rhs);
    foo& operator=(const foo& rhs);
    foo& operator=(foo&& rhs);
    void set(int v);
    int get() const;
};
```



	
	
	
	
	
	
