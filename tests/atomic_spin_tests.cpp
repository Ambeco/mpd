#include "concurrency/atomic_spin.hpp"

#include <future>

std::atomic<unsigned long long> aull(1);

void update_evens_to_odds() {
	std::pair<bool, unsigned long long> r;
	do {
		r = mpd::atomic_exchange_spin(aull, [](auto& value) {
			if (value % 2 == 0) {
				++value;
				return mpd::atomic_exchange_result::store;
			} else
				return mpd::atomic_exchange_result::retry;
		});
	} while (r.first && r.second < 100);
}
void update_odds_to_evens() {
	std::pair<bool, unsigned long long> r;
	do {
		r = mpd::atomic_exchange_spin(aull, [](auto& value) {
			if (value % 2 == 1) {
				++value;
				return mpd::atomic_exchange_result::store;
			} else
				return mpd::atomic_exchange_result::retry;
		});
	} while (r.first && r.second < 100);
}

void test_atomic_spin() {
	// store
	aull = 0;
	auto r = mpd::atomic_exchange_spin(aull, [](auto& value) {++value; return mpd::atomic_exchange_result::store; });
	assert(r.first == true);
	assert(r.second == 1);
	assert(aull == 1);

	// abort
	aull = 0;
	r = mpd::atomic_exchange_spin(aull, [](auto& value) {++value; return mpd::atomic_exchange_result::abort; });
	assert(r.first == false);
	assert(r.second == 0);
	assert(aull == 0);

	// retry
	aull = 0;
	int c = 0;
	r = mpd::atomic_exchange_spin(aull, [&](auto& value) {
		++value;
		if (++c < 10)
			return mpd::atomic_exchange_result::retry;
		else
			return mpd::atomic_exchange_result::store;
			
	});
	assert(r.first == true);
	assert(r.second == 1);
	assert(aull == 1);
	assert(c == 10);

	//async test
	aull = 0;
	std::future<void> odds = std::async(std::launch::async, update_evens_to_odds);
	update_odds_to_evens();
	odds.get();
}