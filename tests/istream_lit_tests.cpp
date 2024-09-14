#include "inputoutput/istream_lit.hpp"
#include <cassert>
#include <sstream>

template<class...read_ts>
void test_literals(const char* input, bool streamstate, read_ts&&... reads) {
	std::istringstream ss(input);
	int t[sizeof...(reads)] = {((ss >> std::forward<read_ts>(reads)),0)...};
	(void) t;
	assert(!!ss == streamstate);
}

template<class fail_t, class then_t>
void test_clearstate(const char* input, fail_t&& failing, then_t&& then) {
	std::istringstream ss(input);
	ss>>std::forward<fail_t>(failing);
	assert(!ss);
	ss.clear();

	ss>>std::forward<then_t>(then);
	assert(ss);
}

template<std::size_t N, class then_t>
void test_mutable(const char* input, bool streamstate, const char* expected, then_t&& then) {
	char buffer[N] {};
	std::istringstream ss(input);

	ss>>buffer;
	assert(!!ss == streamstate);
	assert(std::strcmp(expected, buffer) == 0);

	ss>>std::forward<then_t>(then);
	assert(ss);
}

void test_istream_lit() {
	test_literals("test", true, "test");
	test_literals("testa", true, "test");
	test_literals("test", false, "test?");
	test_literals("test", false, "te?");
	test_literals("test", true, 't', 'e');
	test_literals("test", true, 't', 'e', 's', 't');
	test_literals("test", false, 't', 'e', 's', 't', '?');
	test_literals("test", false, 't', 'e', '?');
	test_literals("test", true, "");

	test_clearstate("test", "te?", "s");
	test_clearstate("test", '?', 't');

	//test_mutable<4>("test", true, "test", "");
	//test_mutable<3>("tes ", true, "tes", " ");
	//test_mutable<3>("test", false, "tes", "t");
}