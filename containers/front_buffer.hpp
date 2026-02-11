#pragma once
#include "algorithms/algorithm.hpp"
#include "iterators/iterator.hpp"
#include "iterators/reference_iterator.hpp"
#include "utilities/macros.hpp"
#include "memory/memory.hpp"
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace mpd {

	enum overflow_behavior_t {
		exception,
		assert,
		truncate
	};
	namespace impl {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmicrosoft-exception-spec"
		template<overflow_behavior_t> std::size_t max_length_check(std::size_t given, std::size_t maximum);
		template<>
		inline std::size_t max_length_check<overflow_behavior_t::exception>(std::size_t given, std::size_t maximum) {
			if (given > maximum) throw std::length_error(std::to_string(given) + " overflows the buffer");
			return given;
		}
		template<>
		inline std::size_t max_length_check<overflow_behavior_t::assert>(std::size_t given, std::size_t maximum) noexcept {
			assume(given < maximum);
			return given;
		}
		template<>
		inline std::size_t max_length_check<overflow_behavior_t::truncate>(std::size_t given, std::size_t maximum) noexcept {
			if (given > maximum) {
				return maximum;
			}
			return given;
		}
#pragma clang diagnostic pop
	}

	namespace impl {
		template <class T, std::size_t alignment = alignof(T), class ForwardIterator>
		void front_buffer_insert_front_unchecked(
			T* buffer,
			std::size_t size,
			std::size_t capacity,
			ForwardIterator first,
			ForwardIterator src_construct_it,
			std::size_t insert_assign,
			std::size_t move_assign,
			std::size_t insert_construct,
			std::size_t move_construct,
			std::size_t insert_total,
			std::size_t move_construct_idx,
			std::size_t final_size
		)
			noexcept(std::is_nothrow_constructible_v<T, decltype(*src_construct_it)>
				&& std::is_nothrow_move_constructible_v<T>
				&& std::is_nothrow_move_assignable_v<T>
				&& std::is_nothrow_assignable_v<T, decltype(*src_construct_it)>) {
			assume(is_aligned_array(buffer, capacity, alignment));
			// step 2: validate the plan
			assume(insert_assign + insert_construct == insert_total);
			assume(insert_total <= capacity);
			assume(move_assign + move_construct <= size); // would be equal except for capacity
			assume(insert_assign + move_assign == size);
			assume(size + insert_construct + move_construct == final_size);
			if (insert_construct) assume(move_assign == 0);
			if (move_assign) assume(insert_construct == 0);
			assume(move_construct_idx >= size);
			assume(move_construct_idx <= final_size);
			assume(move_construct_idx == size + insert_construct);
			assume(move_construct_idx + move_construct == final_size);
			assume(final_size <= capacity);

			// step 3: execute the plan (linear time)
			if (insert_construct) {
				std::uninitialized_copy_n(src_construct_it, insert_construct, buffer + size);
			}
			if (move_construct) {
				try {
					uninitialized_move_n(buffer + move_assign, move_construct, buffer + move_construct_idx);
				} catch (...) { //strong exception guarantee
					destroy(buffer + size, buffer + insert_construct);
					throw;
				}
			}
			try {
				if (move_assign) {
					move_backward_n(buffer+move_assign, move_assign, buffer + size);
				}
				std::copy_n(first, insert_assign, buffer);
			} catch (...) { //weak exception guarantee
				destroy(buffer + size, buffer + insert_construct + move_construct);
				throw;
			}
		}

		template <overflow_behavior_t overflow, class T, std::size_t alignment = alignof(T), class ForwardIterator>
		std::size_t front_buffer_insert_front(T* buffer, std::size_t size, std::size_t capacity, ForwardIterator src_first, ForwardIterator src_last, std::forward_iterator_tag dispatch_tag)
			noexcept(noexcept(max_length_check<overflow>(0, 0))
				&& noexcept(front_buffer_insert_front_unchecked<T, alignment>(buffer, size, capacity, src_first, src_last, 0, 0, 0, 0, 0, 0, 0))) {
			// despite the line count, this is only a single read pass (std::distance) and a single write pass.
			assume(is_aligned_array(buffer, capacity, alignment));
			assert(capacity <= std::numeric_limits<std::size_t>::max() / 4u / sizeof(T));
			assume(size <= capacity);

			// step 1: count how many to assign / insert / move (linear time for forward-iterators, constant time for random access iterators)
			typename std::iterator_traits<ForwardIterator>::iterator_category tag{};
			auto assign_it_and_count = next_up_to_n(src_first, src_last, size, tag);
			ForwardIterator src_construct_it = assign_it_and_count.first;
			std::size_t insert_assign = assign_it_and_count.second;
			auto construct_it_and_count = next_up_to_n(src_construct_it, src_last, capacity - size, tag);
			ForwardIterator src_end_consntruct_it = construct_it_and_count.first;
			std::size_t insert_construct = construct_it_and_count.second;
			std::size_t insert_total = insert_assign + insert_construct;
			if (src_end_consntruct_it != src_last) {
				max_length_check<overflow>(insert_total + 1, capacity);
			}
			std::size_t move_assign = size - insert_assign;
			std::size_t move_construct = std::min(insert_assign, capacity - size - insert_construct);
			std::size_t final_size = size + insert_construct + move_construct;
			std::size_t move_construct_idx = size + insert_construct;

			// step 2: validate the plan
			// step 3: execute the plan (linear time)
			front_buffer_insert_front_unchecked<T, alignment>(buffer, size, capacity, src_first, src_construct_it, insert_assign, move_assign, insert_construct, move_construct, insert_total, move_construct_idx, final_size);
			return final_size;
		}

		template <overflow_behavior_t overflow, class T, std::size_t alignment = alignof(T), class ForwardIterator>
		std::size_t front_buffer_insert_front(T* buffer, std::size_t size, std::size_t capacity, ForwardIterator first, ForwardIterator last, std::input_iterator_tag)
			noexcept(noexcept(max_length_check<overflow>(0, 0))
				&& std::is_nothrow_constructible_v<T, decltype(*first)>
				&& std::is_nothrow_move_constructible_v<T>
				&& std::is_nothrow_move_assignable_v<T>
				&& std::is_nothrow_assignable_v<T, decltype(*first)>) {
			// we can't know the insert size beforehand, so this gets trickier. This usually takes 3-4 write passes.
			assume(is_aligned_array(buffer, capacity, alignment));
			assert(size < capacity);

			// step 1: append the new items to the data already in the buffer
			//std::size_t max_construct_count = capacity - size;
			auto end_construct_its = uninitialized_copy_s(first, last, buffer + size, buffer + capacity);
			try {
				assume(end_construct_its.second >= buffer);
				assume(end_construct_its.second <= buffer + capacity);
				first = end_construct_its.first;
				std::size_t new_size = end_construct_its.second - buffer;
				assume(new_size <= capacity);

				// step 2: rotate the inserted items to the front
				std::rotate(buffer, buffer + size, buffer + new_size);
				std::size_t pre_overwrite_size = new_size - size;
				assume(pre_overwrite_size <= capacity);

				// step 3: capacity check
				if (first == last) {
					return new_size;
				} else {
					max_length_check<overflow>(capacity + 1, capacity);
					if (size == 0) {
						return new_size;
					}
				}

				// step 4: If we got this far, then that means we need to truncate the "old" items, but we still don't know how many to truncate.
				// we'll simply overwrite from the back to the front, so that overwrites occur from lastmost of old content to frontmost.
				assume(new_size == capacity);
				auto end_assign_its = copy_s(first, last, std::make_reverse_iterator(buffer + capacity), std::make_reverse_iterator(buffer + pre_overwrite_size));
				T* end_overwrite = end_assign_its.second.base();
				assume(end_overwrite >= buffer + pre_overwrite_size);
				assume(end_overwrite <= buffer + capacity);
				// step 5: rotate the overwritten elements to the middle, and the remaining original elements to the back
				auto begin_original = std::rotate(buffer + pre_overwrite_size, end_overwrite, buffer + capacity);
				assume(begin_original >= buffer + pre_overwrite_size);
				assume(begin_original <= buffer + capacity);
				// step 6: The overwritten elements are in reverse order, so flip them around
				std::reverse(buffer+ pre_overwrite_size, begin_original);
				return capacity;
			} catch (...) {
				// at this point, the strong exception guarantee is a lost cause, but we can provide the weak exception guarantee
				destroy(buffer + size, end_construct_its.second);
				throw;

			}
		}
	}

	template<overflow_behavior_t overflow, class T, std::size_t alignment = alignof(T)>
	std::size_t front_buffer_append_value_construct(T* buffer, std::size_t size, std::size_t capacity, std::size_t count)
		noexcept(noexcept(impl::max_length_check<overflow>(0, 0)) && std::is_nothrow_constructible_v<T>) {
		assume(is_aligned_array(buffer, capacity, alignment));
		std::size_t new_size = impl::max_length_check<overflow>(size + count, capacity);
		uninitialized_value_construct(buffer + size, buffer + new_size);
		return new_size;
	}

	template<overflow_behavior_t overflow, class T, std::size_t alignment = alignof(T)>
	std::size_t front_buffer_append_default_construct(T* buffer, std::size_t size, std::size_t capacity, std::size_t count)
		noexcept(noexcept(impl::max_length_check<overflow>(0, 0)) && std::is_nothrow_constructible_v<T>) {
		assume(is_aligned_array(buffer, capacity, alignment));
		std::size_t new_size = impl::max_length_check<overflow>(size + count, capacity);
		uninitialized_default_construct(buffer + size, buffer + new_size);
		return new_size;
	}

	template<overflow_behavior_t overflow, class T, std::size_t alignment = alignof(T), class...Args>
	std::size_t front_buffer_emplace_back(T* buffer, std::size_t size, std::size_t capacity, Args&&...args)
		noexcept(noexcept(impl::max_length_check<overflow>(0, 0)) && std::is_nothrow_constructible_v<T, Args...>) {
		assume(is_aligned_array(buffer, capacity, alignment));
		assume(size <= capacity);
		if (size != impl::max_length_check<overflow>(size + 1, capacity)) {
			construct_at<T>(buffer + size, std::forward<Args>(args)...);
		}
		return size + 1;
	}

	template<overflow_behavior_t overflow, class T, std::size_t alignment = alignof(T), class SourceIt>
	std::size_t front_buffer_append(T* buffer, std::size_t size, std::size_t capacity, SourceIt src_first, SourceIt src_last)
		noexcept(noexcept(impl::max_length_check<overflow>(0, 0))
			&& noexcept(std::is_constructible_v<T, decltype(*src_first)>)
			&& std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<SourceIt>::iterator_category>) {
		assume(is_aligned_array(buffer, capacity, alignment));
		assume(size <= capacity);
		// this branch is evaluated at compile time, so only one or the other exists in bytecode
		if (std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<SourceIt>::iterator_category>) {
			std::size_t src_count = distance_up_to_n(src_first, src_last, capacity - size + 1);
			std::size_t new_size = impl::max_length_check<overflow>(size + src_count, capacity);
			assume(new_size <= capacity);
			assume(new_size >= size);
			std::size_t append_size = new_size - size;
			std::uninitialized_copy_n(src_first, append_size, buffer + size);
			return new_size;
		} else {
			auto its = uninitialized_copy_s(src_first, src_last, buffer + size, buffer + capacity);
			assume(its.second >= buffer + size);
			assume(its.second <= buffer + capacity);
			try {
				if (its.first != src_last) {
					impl::max_length_check<overflow>(capacity + 1, capacity);
				}
				return its.second - buffer;
			} catch (...) {
				destroy(buffer + size, its.second);
				[&]() {throw; }();
				return 0;
			}
		}
	}

	template <class T, std::size_t alignment = alignof(T)>
	std::size_t front_buffer_pop_back(T* buffer, std::size_t size, std::size_t erase_count) noexcept {
		assume(is_aligned_ptr(buffer, alignment));
		assume(erase_count <= size);
		std::size_t final_size = size - erase_count;
		destroy(buffer + final_size, buffer + size);
		return final_size;
	}

	template <overflow_behavior_t overflow, class T, std::size_t alignment = alignof(T), class ForwardIterator>
	std::size_t front_buffer_insert(T* buffer, std::size_t size, std::size_t capacity, std::size_t pos, ForwardIterator src_first, ForwardIterator src_last)
		noexcept(noexcept(impl::front_buffer_insert_front<overflow, T, alignment>(buffer, size, capacity, src_first, src_last, typename std::iterator_traits<ForwardIterator>::iterator_category{}))) {
		assume(is_aligned_array(buffer, capacity, alignment));
		assume(pos <= size);
		assume(size <= capacity);
		return pos + impl::front_buffer_insert_front<overflow, T, alignment>(buffer + pos, size - pos, capacity - pos, src_first, src_last, typename std::iterator_traits<ForwardIterator>::iterator_category());
	}

	template <class T, std::size_t alignment = alignof(T)>
	std::size_t front_buffer_erase(T* buffer, std::size_t size, std::size_t pos, std::size_t erase_count) noexcept(std::is_nothrow_move_assignable_v<T>) {
		assume(is_aligned_ptr(buffer, alignment));
		assume(pos <= size);
		assume(pos + erase_count <= size);
		std::size_t final_size = size - erase_count;
		std::move(buffer + pos + erase_count, buffer + size, buffer + pos);
		destroy(buffer + final_size, buffer + size);
		return final_size;
	}

	template<overflow_behavior_t overflow, class T, std::size_t alignment = alignof(T), class Predicate>
	std::size_t front_buffer_erase_if(T* buffer, std::size_t size, Predicate pred)
		noexcept(noexcept(pred(buffer[0]) && std::is_move_assignable_v<T>)) {
		assume(is_aligned_array(buffer, capacity, alignment));
		T* newEnd = std::remove_if(buffer, buffer + size, pred);
		assume(newEnd >= buffer);
		assume(newEnd <= buffer + size);
		destroy(buffer + newEnd, buffer + size);
		return newEnd - buffer;
	}


	template<overflow_behavior_t overflow, class T, std::size_t alignment = alignof(T), class...Args>
	std::size_t front_buffer_emplace(T* buffer, std::size_t size, std::size_t capacity, const std::size_t pos, Args&&...args)
		noexcept(noexcept(impl::max_length_check<overflow>(0, 0))
			&& std::is_nothrow_constructible_v<T, Args...>
			&& std::is_nothrow_move_constructible_v<T>
			&& std::is_nothrow_move_assignable_v<T>) {
		assume(is_aligned_array(buffer, capacity, alignment));
		if (pos == size) {
			return front_buffer_emplace_back<overflow, T, alignment>(buffer, size, capacity, std::forward<Args>(args)...);
		} else {
			T t(std::forward<Args>(args)...);
			return front_buffer_insert<overflow, T, alignment>(buffer, size, capacity, pos, std::make_move_iterator(&t), std::make_move_iterator(&t + 1));
		}
	}

	template <overflow_behavior_t overflow, class T, std::size_t alignment = alignof(T), class SourceIterator>
	std::size_t front_buffer_replace(T* buffer, std::size_t size, std::size_t capacity, std::size_t pos, std::size_t replace_count, SourceIterator source_begin, SourceIterator source_end)
		noexcept(noexcept(impl::front_buffer_insert_front<overflow, T, alignment>(buffer, size, capacity, SourceIterator(), SourceIterator(), typename std::iterator_traits<SourceIterator>::iterator_category()))) {
		typename std::iterator_traits<SourceIterator>::iterator_category tag{};
		assume(is_aligned_array(buffer, capacity, alignment));
		assume(size <= capacity);
		assume(pos <= size);
		assume(pos + replace_count <= size);
		T* replace_end = nullptr;
		// TODO: use tag + distance to optimize capacity check
		std::tie(source_begin, replace_end) = copy_s(source_begin, source_end, buffer + pos, buffer + pos + replace_count);
		assume(replace_end >= buffer + pos);
		assume(replace_end <= buffer + pos + replace_count);
		std::size_t next_idx = replace_end - buffer;
		if (source_begin != source_end) { // source is bigger than replacement. need to insert
			return next_idx + impl::front_buffer_insert_front<overflow, T, alignment>(buffer + next_idx, size - next_idx, capacity - next_idx, source_begin, source_end, tag);
		} else if (replace_end != buffer + pos + replace_count) { // source is smaller than replacement
			std::size_t replaced = next_idx - pos;
			return front_buffer_erase<T, alignment>(buffer, size, next_idx, replace_count - replaced);
		} else { // source was the same size as replacement. All done.
			return size;
		}
	}

	namespace impl {
		template<class T, class size_t = std::size_t, std::size_t alignment_ = alignof(T)>
		class front_buffer_reference_state {
			T* buffer;
			size_t* sz;
			size_t max;
		protected:
			static const bool copy_ctor_should_assign = false;
			static const bool move_ctor_should_assign = false;
			static const bool copy_assign_should_assign = true;
			static const bool move_assign_should_assign = false;
			static const bool dtor_should_destroy = false;
			static const std::size_t alignment = alignment_;
			void set_size(size_t s) noexcept { *sz = s; }
		public:
			using value_type = T;
			using size_type = size_t;
			front_buffer_reference_state(T* buffer_, size_t* size_, size_t capacity_) noexcept :buffer(buffer_), sz(size_), max(capacity_)
			{ assume(is_aligned_array(buffer, max, alignment)); }
			front_buffer_reference_state(const front_buffer_reference_state& rhs) noexcept :buffer(rhs.data()), sz(rhs.size()), max(rhs.max)
			{ assume(is_aligned_array(buffer, max, alignment)); }
			front_buffer_reference_state(front_buffer_reference_state&& rhs) noexcept :buffer(rhs.data()), sz(rhs.size()), max(rhs.max) 
			{ assume(is_aligned_array(buffer, max, alignment)); }

			front_buffer_reference_state& operator=(front_buffer_reference_state&& rhs) noexcept {
				buffer = rhs.buffer;
				sz = rhs.size;
				max = rhs.max;
				return *this;
			}
			template<class U, class size_t2, std::size_t align2>
			front_buffer_reference_state& operator=(const front_buffer_reference_state<U, size_t2, align2>& rhs) noexcept {}
			~front_buffer_reference_state() = default;
			T* data() noexcept { assume(is_aligned_array(buffer, max, alignment)); return buffer; }
			const T* data() const noexcept { assume(is_aligned_array(buffer, max, alignment)); return buffer; }
			size_t size() const noexcept { return *sz; }
			size_t capacity() const noexcept { assume(is_aligned_array(buffer, max, alignment)); return max; }
			std::allocator<T> get_allocator() const { return {}; }
		};

		// TODO: only overalign if trivial
		template<class T, std::size_t capacity_, std::size_t alignment_ = std::max(alignof(T), alignof(max_align_t))>
		class front_buffer_array_state {
		public:
			using value_type = T;
			using size_type = std::conditional_t<(capacity_ >= 255), unsigned short, unsigned char>;
			static_assert(capacity_ < std::numeric_limits<size_type>::max(), "capacity doesn't fit in size_type");
		protected:
			static const bool copy_ctor_should_assign = true;
			static const bool move_ctor_should_assign = true;
			static const bool copy_assign_should_assign = true;
			static const bool move_assign_should_assign = true;
			static const bool dtor_should_destroy = false;
			static const std::size_t alignment = alignment_;
			static const std::size_t aligned_capacity_ = ((sizeof(T) * capacity_ + alignment_ - 1) / alignment_ * alignment_);
		private:
			size_type sz;
			union data {
				char no_construct;
				alignas(alignment_) T buffer[aligned_capacity_];
				data() {}
				~data() {}
			} d;
			// ensure that overaligned bytes are zeroed out, so that algorithms can read/write aligned blocks deterministically.
			void init_overaligned() noexcept {
				if (std::is_trivially_copyable_v<T>)
					std::memset(d.buffer + capacity_, 0, sizeof(T)*(aligned_capacity() - capacity_)); 
			}
		protected:
			void set_size(size_type s) noexcept {  assume(s <= capacity_); sz = s; }
		public:
			front_buffer_array_state() noexcept :sz(0) { init_overaligned(); }
			front_buffer_array_state(const front_buffer_array_state& rhs) noexcept : sz(0) { init_overaligned(); }
			template<class U, std::size_t capacity2, std::size_t align2>
			front_buffer_array_state(const front_buffer_array_state<U, capacity2, align2>& rhs) noexcept :sz(0) { init_overaligned(); }
			front_buffer_array_state& operator=(const front_buffer_array_state& rhs) noexcept { return *this; }
			template<class U, std::size_t capacity2, std::size_t align2>
			front_buffer_array_state& operator=(const front_buffer_array_state<U, capacity2, align2>& rhs) noexcept { return *this; }
			~front_buffer_array_state() { }
			T* data() noexcept { assume(is_aligned_array(d.buffer, aligned_capacity_, alignment)); return d.buffer; }
			const T* data() const noexcept { assume(is_aligned_array(d.buffer, aligned_capacity_, alignment)); return d.buffer; }
			size_type size() const noexcept { return sz; }
			size_type capacity() const noexcept { return capacity_; }
			size_type aligned_capacity() const noexcept { assume(is_aligned_array(d.buffer, aligned_capacity_, alignment)); return capacity_; }
			std::allocator<T> get_allocator() const { return {}; }
		};

		template<class T, class Allocator, std::size_t alignment_ = alignof(T)>
		class front_buffer_heap_state : Allocator::template rebind<T>::type{
		public:
			using value_type = T;
			using size_type = std::size_t;
			using allocator = typename Allocator::template rebind<T>::type;
			static const bool copy_ctor_should_assign = true;
			static const bool move_ctor_should_assign = false;
			static const bool copy_assign_should_assign = true;
			static const bool move_assign_should_assign = false;
			static const bool dtor_should_destroy = false;
			static const std::size_t alignment = alignment_;
		private:
			size_type max;
			size_type sz;
			T* buffer;
			// ensure that overaligned bytes are zeroed out, so that algorithms can read/write aligned blocks deterministically.
			void init_overaligned() noexcept { 
				if (std::is_trivially_copyable_v<T>)
					std::memset(buffer + max, 0, sizeof(T)*(aligned_capacity()-capacity));
			}
		protected:
			void set_size(size_type s) noexcept { sz = s; }
			static size_type aligned_capacity(size_type capcity) {
				return (sizeof(T) * capcity + alignment_ - 1) / alignment_ * alignment_;
			}
		public:
			explicit front_buffer_heap_state(size_type capacity_)
				:max(capacity_), sz(0), buffer(allocate(aligned_capacity(capacity))) { init_overaligned(); }
			explicit front_buffer_heap_state(size_type capacity_, const Allocator& a)
				:allocator(a), max(capacity_), sz(0), buffer(a.allocate(aligned_capacity(capacity))) { init_overaligned(); }
			template<class U, class Allocator2, std::size_t align2>
			front_buffer_heap_state(const front_buffer_heap_state<U, Allocator2, align2>& rhs)
				: allocator(rhs), max(rhs.max), sz(0), buffer(allocate(rhs.aligned_capacity())) { init_overaligned(); }
			front_buffer_heap_state(front_buffer_heap_state&& rhs)
				:allocator(rhs), max(rhs.max), sz(rhs.sz), buffer(rhs.buffer) 
			{ rhs.max = 0; rhs.sz = 0; rhs.buffer = nullptr; }
			~front_buffer_heap_state() { allocator::deallocate(buffer); }
			template<class U, class Allocator2, std::size_t align2>
			front_buffer_heap_state& operator=(const front_buffer_heap_state<U, Allocator2, align2>& rhs) {
				allocator::operator=(rhs); return *this;
			}
			front_buffer_heap_state& operator=(front_buffer_heap_state&& rhs) {
				allocator::operator=(rhs);
				std::swap(max, rhs.max);
				std::swap(sz, rhs.sz);
				std::swap(buffer, rhs.buffer);
				return *this;
			}
			T* data() noexcept { assume(is_aligned_ptr(buffer, alignment));  return buffer; }
			const T* data() const noexcept { assume(is_aligned_ptr(buffer, alignment)); return buffer; }
			size_type size() const noexcept { return sz; }
			size_type capacity() const noexcept { return max; }
			size_type aligned_capacity() const noexcept { return aligned_capacity(max); }
			std::allocator<T> get_allocator() const { return {}; }
		};

	}

	// base class for buffers and buffer_references.
	template<class state, overflow_behavior_t overflow>
	class basic_front_buffer : public state {
	protected:
		using T = typename state::value_type;
		static const bool copy_ctor_should_assign = state::copy_ctor_should_assign;
		static const bool move_ctor_should_assign = state::move_ctor_should_assign;
		static const bool copy_assign_should_assign = state::copy_assign_should_assign;
		static const bool move_assign_should_assign = state::move_assign_should_assign;
		static const bool dtor_should_destroy = state::dtor_should_destroy;
		static const std::size_t alignment = state::alignment;
		T* d() noexcept { return state::data(); }
		const T* d() const noexcept { return state::data(); }
		typename state::size_type s() const noexcept { return state::size(); }
		typename state::size_type c() const noexcept { return state::capacity(); }
		void sets(std::size_t s) noexcept { return this->set_size(static_cast<typename state::size_type>(s)); }
	public:
		using value_type = T;
		using size_type = typename state::size_type;
		using difference_type = ptrdiff_t;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;
		using iterator = T*;
		using const_iterator = const T*;
		using reverse_iterator = std::reverse_iterator<T*>;
		using const_reverse_iterator = std::reverse_iterator<const T*>;
		using buffer_reference = basic_front_buffer<impl::front_buffer_reference_state<T, size_type, alignment>, overflow>;

		basic_front_buffer() noexcept(noexcept(state())) { sets(0); }
		explicit basic_front_buffer(state&& s) noexcept(noexcept(state(s))) : state(s) {}
		basic_front_buffer(size_type count, const T& value) { assign(count, value); }
		basic_front_buffer(state&& s, size_type count, const T& value) : state(std::move(s)) { assign(count, value); }
		basic_front_buffer(size_type count) { resize(count); }
		basic_front_buffer(state&& s, size_type count) : state(std::move(s)) { resize(count); }
		template<class InputIt, bool is_convertible = std::is_convertible_v<typename std::iterator_traits<InputIt>::value_type, T>>
		basic_front_buffer(InputIt first, InputIt last) { assign(first, last); }
		template<class InputIt, bool is_convertible = std::is_convertible_v<typename std::iterator_traits<InputIt>::value_type, T>>
		basic_front_buffer(state&& s, InputIt first, InputIt last) : state(std::move(s)) { assign(first, last); }
		basic_front_buffer(const basic_front_buffer& rhs) :state(rhs) {
			if (copy_ctor_should_assign) assign(rhs.begin(), rhs.end());
		}
		basic_front_buffer(basic_front_buffer&& rhs) :state(std::move(rhs)) {
			if (move_ctor_should_assign) assign(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
		}
		basic_front_buffer(std::initializer_list<T> init) { assign(init.begin(), init.end()); }
		//basic_front_buffer(state&& s, std::initializer_list<T> init) :state(std::move(s)) { assign(init.begin(), init.end()); }
		template<overflow_behavior_t overflow2>
		basic_front_buffer(basic_front_buffer<state, overflow2>&& rhs) :state(std::move(rhs)) {
			if (copy_ctor_should_assign) assign(rhs.begin(), rhs.end());
		}
		template<class state2, overflow_behavior_t overflow2, bool is_convertible = std::is_convertible_v<typename state2::value_type, T>>
		basic_front_buffer(const basic_front_buffer<state2, overflow2>& rhs) :state(rhs) {
			if (copy_ctor_should_assign) assign(rhs.begin(), rhs.end());
		}
		template<class state2, overflow_behavior_t overflow2, bool is_convertible = std::is_convertible_v<typename state2::value_type, T>>
		basic_front_buffer(basic_front_buffer<state2, overflow2>&& rhs) :state(rhs) {
			assign(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
		}
		//template<class state2, overflow_behavior_t overflow2, bool is_convertible = std::is_convertible_v<typename state2::value_type, T>>
		//basic_front_buffer(state&& s, const basic_front_buffer<state2, overflow2>& rhs) :state(std::move(s)) {
		//	assign(rhs.begin(), rhs.end());
		//}
		//template<class state2, overflow_behavior_t overflow2, bool is_convertible = std::is_convertible_v<typename state2::value_type, T>>
		//basic_front_buffer(state&& s, basic_front_buffer<state2, overflow2>&& rhs) :state(std::move(s)) {
		//	assign(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
		//}
		template<class U, bool is_convertible = std::is_convertible_v<U, T>>
		basic_front_buffer(const std::vector<U>& rhs) {
			assign(rhs.begin(), rhs.end());
		}
		template<class U, bool is_convertible = std::is_convertible_v<U, T>>
		basic_front_buffer(std::vector<U>&& rhs) {
			assign(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
		}
		~basic_front_buffer() noexcept { if (dtor_should_destroy) clear(); }

		basic_front_buffer& operator=(const basic_front_buffer& rhs)
			noexcept(std::is_nothrow_assignable_v<state, typename state::value_type> && (!copy_assign_should_assign || std::is_nothrow_assignable_v<T, typename state::value_type>)) {
			state::operator=(rhs);
			if (copy_assign_should_assign)
				assign(rhs.begin(), rhs.end());
			return *this;
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t< std::is_convertible_v<typename state2::value_type, T>, basic_front_buffer&>
			operator=(const basic_front_buffer<state2, overflow2>& rhs)
			noexcept(std::is_nothrow_assignable_v<state, typename state::value_type> && (!copy_assign_should_assign || std::is_nothrow_assignable_v<T, typename state::value_type>)) {
			state::operator=(rhs);
			if (copy_assign_should_assign)
				assign(rhs.begin(), rhs.end());
			return *this;
		}
		template<overflow_behavior_t overflow2>
		basic_front_buffer& operator=(basic_front_buffer<state, overflow2>&& rhs)
			noexcept(std::is_nothrow_move_assignable_v<state> && (!move_assign_should_assign || std::is_nothrow_move_assignable_v<T>)) {
			state::operator=(std::move(rhs));
			if (move_assign_should_assign)
				assign(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
			return *this;
		}
		template<class state2, overflow_behavior_t overflow2>
		std::enable_if_t< std::is_convertible_v<typename state2::value_type, T>, basic_front_buffer&>
			operator=(basic_front_buffer<state2, overflow2>&& rhs)
			noexcept(std::is_nothrow_copy_assignable_v<state>&& std::is_nothrow_move_assignable_v<T>) {
			state::operator=(rhs);
			assign(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
			return *this;
		}
		template<class Container, class T2=decltype(*std::begin(std::declval<Container>()))>
		std::enable_if_t<std::is_assignable_v<T, T2> && std::is_constructible_v<T,T2>, basic_front_buffer&>
			operator=(const Container& rhs)
			noexcept(std::is_nothrow_assignable_v<T, T2> && std::is_nothrow_constructible_v<T, T2>) {
			assign(rhs.begin(), rhs.end());
			return *this;
		}
		template<class Container, class T2=decltype(*std::begin(std::declval<Container>()))>
		std::enable_if_t<std::is_assignable_v<T, T2&&> && std::is_constructible_v<T, T2&&>, basic_front_buffer&>
			operator=(Container&& rhs)
			noexcept(std::is_nothrow_assignable_v<T, T2&&>&& std::is_nothrow_constructible_v<T, T2&&>) {
			assign(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
			return *this;
		}
		template<class value_type>
		std::enable_if_t< std::is_convertible_v<value_type, T>, basic_front_buffer&>
			operator=(std::initializer_list<value_type> rhs) noexcept(noexcept(assign(rhs.begin(), rhs.end()))) {
			assign(rhs.begin(), rhs.end()); return *this;
		}

		operator buffer_reference() noexcept { return buffer_reference(*this); }
		template<class SrcIt>
		std::enable_if_t< std::is_convertible_v<typename std::iterator_traits<SrcIt>::value_type, T>, void>
			assign(SrcIt first, SrcIt last) noexcept(noexcept(front_buffer_replace<overflow>(d(), s(), c(), 0, s(), first, last))) {
			sets(front_buffer_replace<overflow>(d(), s(), c(), 0, s(), first, last));
		}
		void assign(size_type count, const T& value) { assign(ref_iter(value, 0), ref_iter(value, count)); }
		void assign(std::initializer_list<T> iList) { assign(iList.begin(), iList.end()); }
		reference at(size_type pos) {
			if (pos >= size()) throw std::out_of_range(std::to_string(pos) + " bigger than size " + std::to_string(size()));
			return d()[pos];
		}
		const_reference at(size_type pos) const {
			if (pos >= size()) throw std::out_of_range(std::to_string(pos) + " bigger than size " + std::to_string(size()));
			return d()[pos];
		}
		reference operator[](size_type pos) noexcept { assume(pos < s()); return d()[pos]; }
		const_reference operator[](size_type pos) const noexcept { assume(pos < size()); return d()[pos]; }
		using state::data;
		reference front() noexcept { assume(size > 0); return d()[0]; }
		const_reference front() const noexcept { assume(s() > 0); return d()[0]; }
		reference back() noexcept { assume(size > 0); return d()[size() - 1]; }
		const_reference back() const noexcept { assume(s() > 0); return d()[s() - 1]; }
		iterator begin() noexcept { return d(); }
		const_iterator begin() const noexcept { return d(); }
		const_iterator cbegin() const noexcept { return d(); }
		iterator end() noexcept { return d() + s(); }
		const_iterator end() const noexcept { return d() + s(); }
		const_iterator cend() const noexcept { return d() + s(); }
		reverse_iterator rbegin() noexcept { return reverse_iterator{d() + s()}; }
		const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{d() + s()}; }
		const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator{d() + s()}; }
		reverse_iterator rend() noexcept { return reverse_iterator{d()}; }
		const_reverse_iterator rend() const noexcept { return const_reverse_iterator{d()}; }
		const_reverse_iterator crend() const noexcept { return const_reverse_iterator{d()}; }
		bool empty() const noexcept { return s() == 0; }
		using state::size;
		using state::capacity;
		void clear() noexcept { sets(front_buffer_pop_back(d(), s(), s())); }
		size_type max_size() const noexcept { return capacity(); }
		void reserve(size_type new_cap) { if (new_cap > capacity()) throw std::length_error("front_buffer#resize(" + std::to_string(new_cap) + ") bigger than " + std::to_string(capacity())); }
		void shrink_to_fit() noexcept {}
		template<class SrcIt>
		std::enable_if_t< std::is_convertible_v<typename std::iterator_traits<SrcIt>::value_type, T>, iterator>
			insert(const_iterator pos, SrcIt first, SrcIt last) {
			T* d = data();
			sets(front_buffer_insert<overflow>(d, s(), c(), pos - d, first, last));
			return d + (pos - d);
		}
		iterator insert(const_iterator pos, const T& value) { return insert(pos, ref_iter(value, 0), ref_iter(value, 1)); }
		iterator insert(const_iterator pos, T&& value) { return insert(pos, std::make_move_iterator(ref_iter(value, 0)), std::make_move_iterator(ref_iter(value, 1))); }
		iterator insert(const_iterator pos, size_type count, const T& value) { return insert(pos, ref_iter(value, 0), ref_iter(value, count)); }
		iterator insert(const_iterator pos, std::initializer_list<T> iList) { return insert(pos, iList.begin(), iList.end()); }
		template<class...Args>
		std::enable_if_t<std::is_constructible_v<T, Args...>, iterator>
			emplace(const_iterator pos, Args&&...args) {
			T* d = data();
			sets(front_buffer_emplace<overflow>(d, s(), c(), pos-d, std::forward<Args>(args)...));
			return d + (pos - d);
		}
		iterator erase(const_iterator first, const_iterator last) {
			T* d = data();
			sets(front_buffer_erase(d, s(), first - d, last - first));
			return d + (first - d);
		}
		iterator erase(const_iterator first) { return erase(first, first + 1); }
		void push_back(const T& v) { emplace_back(v); }
		void push_back(T&& v) { emplace_back(std::move(v)); }
		template<class...Args>
		std::enable_if_t< std::is_constructible_v<T, Args...>, reference>
			emplace_back(Args&&...args) {
			T* d = data();
			sets(front_buffer_emplace_back<overflow>(d, s(), c(), std::forward<Args>(args)...));
			return d[s() - 1];
		}
		void pop_back() noexcept { sets(front_buffer_pop_back(d(), s(), 1)); }
		void resize(size_type want_s) {
			size_type s = size();
			if (want_s > s) {
				sets(front_buffer_append_value_construct<overflow>(d(), s, c(), want_s - s));
			} else {
				sets(front_buffer_pop_back(d(), s, s - want_s));
			}
		}
		void resize(size_type want_s, const T& value) {
			size_type s = size();
			if (want_s > s) {
				sets(front_buffer_append<overflow>(d(), s, c(), ref_iter(value), ref_iter(value, s)));
			} else {
				sets(front_buffer_pop_back(d(), s, s - want_s));
			}
		}
		void resize_default_construct(size_type want_s) {
			size_type s = size();
			if (want_s > s) {
				sets(front_buffer_append_default_construct<overflow>(d(), s, c(), want_s - s));
			} else {
				sets(front_buffer_pop_back(d(), s, s - want_s));
			}
		}
		//no swap method, since that'd unexpectedly be O(n) on most implementations
	};
	template<class state1, overflow_behavior_t overflow1, class state2, overflow_behavior_t overflow2>
	bool operator==(const basic_front_buffer<state1, overflow1>& l, const basic_front_buffer<state2, overflow2>& r) noexcept {
		return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin());
	}
	template<class state1, overflow_behavior_t overflow1, class state2, overflow_behavior_t overflow2>
	bool operator!=(const basic_front_buffer<state1, overflow1>& l, const basic_front_buffer<state2, overflow2>& r) noexcept {
		return !operator==(l, r);
	}
	template<class state1, overflow_behavior_t overflow1, class state2, overflow_behavior_t overflow2>
	bool operator<(const basic_front_buffer<state1, overflow1>& l, const basic_front_buffer<state2, overflow2>& r) noexcept {
		return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
	}
	template<class state1, overflow_behavior_t overflow1, class state2, overflow_behavior_t overflow2>
	bool operator<=(const basic_front_buffer<state1, overflow1>& l, const basic_front_buffer<state2, overflow2>& r) noexcept {
		return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end(), std::less_equal<typename state1::value_type>{});
	}
	template<class state1, overflow_behavior_t overflow1, class state2, overflow_behavior_t overflow2>
	bool operator>(const basic_front_buffer<state1, overflow1>& l, const basic_front_buffer<state2, overflow2>& r)  noexcept {
		return !operator<=(l, r);
	}
	template<class state1, overflow_behavior_t overflow1, class state2, overflow_behavior_t overflow2>
	bool operator>=(const basic_front_buffer<state1, overflow1>& l, const basic_front_buffer<state2, overflow2>& r) noexcept {
		return !operator<(l, r);
	}
	template<class state, mpd::overflow_behavior_t overflow>
	constexpr std::size_t erase(mpd::basic_front_buffer<state, overflow>& c, const typename state::value_type& value) {
		auto it = std::remove(c.begin(), c.end(), value);
		auto r = c.end() - it;
		c.erase(it, c.end());
		return r;
	}
	template<class state, mpd::overflow_behavior_t overflow, class Pred>
	constexpr std::size_t erase_if(mpd::basic_front_buffer<state, overflow>& c, Pred pred) {
		auto it = std::remove_if(c.begin(), c.end(), pred);
		auto r = c.end() - it;
		c.erase(it, c.end());
		return r;
	}

	//note that as a reference this is NON owning.
	template<class T, overflow_behavior_t overflow = overflow_behavior_t::exception, class size_t = std::size_t, std::size_t alignment = alignof(T)>
	using buffer_reference = basic_front_buffer<impl::front_buffer_reference_state<T, size_t, alignment>, overflow>;

		// TODO: only overalign if trivial
	template<class T, std::size_t capacity, overflow_behavior_t overflow = overflow_behavior_t::exception, std::size_t alignment = std::max(alignof(T), alignof(max_align_t))>
	using array_buffer = basic_front_buffer<impl::front_buffer_array_state<T, capacity, alignment>, overflow>;

	template<class T, class Allocator = std::allocator<T>, overflow_behavior_t overflow = overflow_behavior_t::exception, std::size_t alignment = alignof(T)>
	using dynamic_buffer = basic_front_buffer<impl::front_buffer_heap_state<T, Allocator, alignment>, overflow>;
}
namespace std {
	template<class state, mpd::overflow_behavior_t overflow>
	class hash<mpd::basic_front_buffer<state, overflow>> {
	public:
		std::size_t operator()(const mpd::basic_front_buffer<state, overflow>& val) const noexcept {
			std::hash<typename state::value_type> subhasher;
			std::size_t ret = 0x9e3779b9;
			for (int i = 0; i < val.size(); i++)
				ret ^= subhasher(val[i]) + 0x9e3779b9 + (ret << 6) + (ret >> 2);
			return ret;
		}
	};
}
