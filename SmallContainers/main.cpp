#include "small_string.h"
#include <iostream>
#include <utility>
#include <cwchar>

using test_string_t = mpd::small_basic_string<wchar_t, 5, mpd::overflow_behavior_t::silently_truncate>;
using bigger_string_t = mpd::small_basic_string<wchar_t, 7, mpd::overflow_behavior_t::silently_truncate>;

struct c_out_it {
	using value_type = wchar_t;
	using difference_type = std::ptrdiff_t;
	using reference = wchar_t&;
	using pointer = wchar_t*;
	using iterator_category = std::output_iterator_tag;
	wchar_t c;
	c_out_it operator++() { ++c; return *this; }
	c_out_it operator++(int) { ++c; return *this; }
	wchar_t operator*() const { return c; }
	bool operator==(c_out_it other) const { return c == other.c; }
	bool operator!=(c_out_it other) const { return c != other.c; }
};

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
_test_mutation(expected_value, [=](test_string_t& v){op;});
#define test_mutate_and(op, check) \
{test_string_t v; op; assert(check);}

void test_def_ctor() {
	test_string_t str{};
	assert(str.size() == 0);
	assert(*str.data() == 0);
}

int main() {
	std::wstring std_string = L"fghijk";
	test_string_t small_lvalue = L"lmnopq";
	bigger_string_t large_lvalue = L"rstuvw";
	std::initializer_list<wchar_t> init_list = { 'x', 'y', 'z', ',', '.', '/' };

	test_def_ctor();
	test_ctor(L"aaaa", 4, L'a');
	test_ctor(L"aaaaa", 7, L'a');
	test_ctor(L"12345", L"1234567");
	test_ctor(L"12345", L"1234567", 7);
	test_ctor(L"12345", L"1234567");
	test_ctor(L"lmnop", small_lvalue);
	test_ctor(L"lmnop", std::move(small_lvalue));
	test_ctor(L"xyz,.", init_list);
	test_ctor(L"fghij", std_string);
	test_ctor(L"ghi", std_string, 1, 3);
	test_ctor(L"k", std_string, 5, 5);
	test_ctor(L"123", bigger_string_t(L"123"));
	test_ctor(L"rstuv", large_lvalue);
	test_ctor(L"stuvw", large_lvalue, 1);
	test_ctor(L"stu", large_lvalue, 1, 3);
	test_ctor(L"w", large_lvalue, 5, 5);
	test_ctor(L"ABCDE", c_out_it{ 'A' }, c_out_it{ 'Z' });
	test_mutation(L"rstuv", v = large_lvalue);
	test_mutation(L"lmnop", v = small_lvalue);
	test_mutation(L"lmnop", v = std::move(small_lvalue));
	test_mutation(L"123", v = L"123");
	test_mutation(L"a", v = L'a');
	test_mutation(L"xyz,.", v = init_list);
	test_mutation(L"fghij", v = std_string);
	test_mutation(L"aaaa", v.assign(4, L'a'));
	test_mutation(L"aaaaa", v.assign(7, L'a'));
	test_mutation(L"rstuv", v.assign(large_lvalue));
	test_mutation(L"stu", v.assign(large_lvalue, 1, 3));
	test_mutation(L"w", v.assign(large_lvalue, 5, 5));
	test_mutation(L"lmnop", v.assign(small_lvalue));
	test_mutation(L"lmnop", v.assign(std::move(small_lvalue)));
	test_mutation(L"12345", v.assign(L"1234567"));
	test_mutation(L"12", v.assign(L"1234567", 2));
	test_mutation(L"xyz,.", v.assign(init_list));
	test_mutation(L"fghij", v.assign(std_string));
	test_mutation(L"ABCDE", v.assign(c_out_it{ 'A' }, c_out_it{ 'Z' }));
	assert(*small_lvalue.begin() == 'l');
	assert(*small_lvalue.cbegin() == 'l');
	assert(*--small_lvalue.end() == 'p');
	assert(*--small_lvalue.cend() == 'p');
	assert(*small_lvalue.rbegin() == 'p');
	assert(*small_lvalue.crbegin() == 'p');
	assert(*--small_lvalue.rend() == 'l');
	assert(*--small_lvalue.crend() == 'l');
	assert(test_string_t(L"a").empty() == false);
	assert(test_string_t(L"").empty() == true);
	assert(test_string_t(L"").size() == 0);
	assert(small_lvalue.size() == 5);
	assert(test_string_t(L"").length() == 0);
	assert(small_lvalue.length() == 5);
	assert(test_string_t().capacity() == 5);
	test_mutate_and(v.reserve(4), v.capacity() == 5);
	test_mutate_and(v.shrink_to_fit(), v.capacity() == 5);
	test_mutate_and(v.assign(L"test").clear(), v.size() == 0);
	test_mutation(L"azzbc", v.insert(1, 2, 'z'));
	test_mutation(L"azzbc", v.insert(1, L"zz"));
	test_mutation(L"azbcd", v.insert(1, L"zz", 1));
	test_mutation(L"arstu", v.insert(1, large_lvalue));
	test_mutation(L"atbcd", v.insert(1, large_lvalue, 2, 1));
	test_mutation(L"abzcd", v.insert(v.begin()+2, 'z'));
	test_mutation(L"abzzc", v.insert(v.begin() + 2, 2, 'z'));
	test_mutation(L"abfgh", v.insert(v.begin() + 2, std_string.begin(), std_string.end()));
	test_mutation(L"abABC", v.insert(v.begin() + 2, c_out_it{ 'A' }, c_out_it{ 'Z' }));
	test_mutation(L"axyz,", v.insert(1, init_list));
	test_mutation(L"aghbc", v.insert(1, std_string, 1, 2));
	test_mutation(L"", v.erase());
	test_mutation(L"ad", v.erase(1, 2));
	test_mutation(L"bcd", v.erase(v.begin()));
	test_mutation(L"cd", v.erase(v.begin(), v.begin()+2));
	test_mutation(L"abcdz", v.push_back('z'));
	test_mutation(L"abc", v.pop_back());
	test_mutation(L"abcdz", v.append(2, 'z'));
	test_mutation(L"abcdr", v.append(large_lvalue));
	test_mutation(L"abcds", v.append(large_lvalue, 1, 2));
	test_mutation(L"abcd1", v.append(L"123", 3));
	test_mutation(L"abcd1", v.append(L"123"));
	test_mutation(L"abcdA", v.append(c_out_it{ 'A' }, c_out_it{ 'Z' }));
	test_mutation(L"abcdx", v.append(init_list));
	test_mutation(L"abcdg", v.append(std_string, 1, 2));

	test_mutation(L"abcdr", v += large_lvalue);
	test_mutation(L"abcda", v += L'a');
	test_mutation(L"abcd1", v += L"123");
	test_mutation(L"abcdx", v += init_list);
	test_mutation(L"abcdf", v += std_string);
	return 0;
}