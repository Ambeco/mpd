#include <atomic>
#include <utility>
#include "../utilities/macros.hpp"

namespace mpd {
	/**
	* return values from atomic_exchange_spin mutation operations.
	*
	* - store: write the value
	* - abort: exit atomic_exchange_spin without updating.
	* - retry: re-read the atomic value and retry the operation.
	**/
	enum class atomic_exchange_result { store, abort, retry };

	/**
	* Update atomic variable with a non-atomic operation, via a spin/retry.
	*
	* The mutating opeoration should take the value by reference, mutate it,
	* and return a atomic_exchange_result.
	*
	* atomic_exchange_spin calls the mutating operation with the current value of the atomic,
	* and then stores the result in the atomic, and returns {true, newValue}.
	* If the atomic's value was changed between read and write, atomic_exchange_spin retries.
	*
	* Takes an atomic by reference, and a mutating operation.
	* Returns a tuple of {did_the_update_complete, newestValue}.
	**/
	template<class T, class F, class...Args>
	std::pair<bool, T> atomic_exchange_spin(std::atomic<T>& atomic, F&& mutation) {
#if __cpp_lib_is_invocable
		static_assert(std::is_invocable_r_v<atomic_bitfield_mutate_step, F, T&>); //gives clearer error messages since C++20
#endif
		T old_value = atomic.load(std::memory_order_consume);
		T store_value;
		for (;;) {
			store_value = old_value;
			auto step = mutation(store_value);
			if (step == atomic_exchange_result::abort) { [[unlikely]] return {false, old_value}; } 
			else if (step == atomic_exchange_result::retry) { [[unlikely]] old_value = atomic.load(std::memory_order_consume); continue; } 
			else if (atomic.compare_exchange_weak(old_value, store_value)) {[[likely]] return {true, store_value}; }
		}
		return {true, store_value};
	}
}