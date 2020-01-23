#include "small_string.h"
#include <iostream>
#include <utility>
#include <cwchar>

using test_string_t = mpd::small_basic_string<wchar_t, 5, mpd::overflow_behavior_t::silently_truncate>;
using bigger_string_t = mpd::small_basic_string<wchar_t, 7, mpd::overflow_behavior_t::silently_truncate>;

template<class...Ts>
void test_ctor(const wchar_t* expected_value, Ts&&... vals) {
	test_string_t str(std::forward<Ts>(vals)...);
	std::size_t found_size = str.size();
	const wchar_t* found_value = str.data();
	assert(found_size == wcslen(expected_value));
	assert(wcscmp(found_value, expected_value)==0);
}

template<class F>
void _test_mutation(const wchar_t* expected_value, F func) {
	test_string_t str(L"abcd");
	func(str);
	std::size_t found_size = str.size();
	const wchar_t* found_value = str.data();
	assert(found_size == wcslen(expected_value));
	assert(wcscmp(found_value, expected_value) == 0);
}
#define test_mutation(expected_value, op) \
_test_mutation(expected_value, [](test_string_t& v){op;});

void test_def_ctor() {
	test_string_t str{};
	assert(str.size() == 0);
	assert(*str.data() == 0);
}

int main() {
	std::wstring std_string = L"abcdefg";
	test_string_t small_lvalue = L"abcdefg";

	test_def_ctor();
	test_ctor(L"aaaa", 4, 'a');
	test_ctor(L"aaaaa", 7, 'a');
	test_ctor(L"abcde", (const wchar_t*)L"abcdefg");
	test_ctor(L"abcde", (const wchar_t*)L"abcdefg", 7);
	test_ctor(L"abcde", (const wchar_t*)L"abcdefg");
	test_ctor(L"abcde", small_lvalue);
	test_ctor(L"abcde", std::move(small_lvalue));
	test_ctor(L"abcde", std::initializer_list<wchar_t>{'a', 'b', 'c', 'd', 'e', 'f', 'g'});
	test_ctor(L"abcde", std_string);
	test_ctor(L"bcd", std_string, 1, 3);
	test_ctor(L"fg", std_string, 5, 5);
	test_ctor(L"abcd", bigger_string_t(L"abcd"));
	test_ctor(L"abcde", bigger_string_t(L"abcdefg"));
	test_ctor(L"bcdef", bigger_string_t(L"abcdefg"), 1);
	test_ctor(L"bcd", bigger_string_t(L"abcdefg"), 1, 3);
	test_ctor(L"fg", bigger_string_t(L"abcdefg"), 5, 5);
	test_mutation(L"abcde", v = bigger_string_t(L"abcdefg"););
	return 0;
}