
#include "containers/initializers.hpp"
#include <cassert>
#include <queue>
#include <vector>

struct test_queue : public std::queue<char, std::vector<char>> {
	using std::queue<char, std::vector<char>>::queue;
	std::size_t capacity() const { return c.capacity(); }
};

void test_initializers() {
	std::vector<unsigned long long> v = mpd::reserved{13};
	assert(v.capacity() == 13);

	test_queue q(mpd::reserved{13});
	assert(q.capacity() == 13);
}