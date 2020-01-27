#define MPD_SSTRING_OVERRRUN_CHECKS
#include "small_string.h"
#include <functional>
#include <iostream>
#include <sstream>
#include <utility>
#include <cwchar>

using test_string_t = mpd::small_basic_string<wchar_t, 5, mpd::overflow_behavior_t::silently_truncate>;
using bigger_string_t = mpd::small_basic_string<wchar_t, 7, mpd::overflow_behavior_t::silently_truncate>;
using char_test_string_t = mpd::small_basic_string<char, 5, mpd::overflow_behavior_t::silently_truncate>;

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
	assert(wcscmp(found_value, expected_value) == 0);
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
#define test_mutate_and(op, check)  {test_string_t v; op; assert(check);}

template<class F>
void _test_big_mutation(const wchar_t* expected_value, F func) {
	bigger_string_t str(L"abcd");
	func(str);
	std::size_t found_size = str.size();
	const wchar_t* found_value = str.data();
	assert(found_size == wcslen(expected_value));
	assert(wcscmp(found_value, expected_value) == 0);
}
#define test_big_mutation(expected_value, op)  _test_big_mutation(expected_value, [=](bigger_string_t& v){op;});

void test_def_ctor() {
	test_string_t str{};
	assert(str.size() == 0);
	assert(*str.data() == 0);
}

std::basic_istringstream<wchar_t> istream(const wchar_t* str) {
	return std::basic_istringstream<wchar_t>(str);
}
using ostream_t = std::basic_ostringstream<wchar_t>;

int main() {
	std::wstring std_string = L"fghijk";
	test_string_t small_lvalue = L"lmnopq";
	bigger_string_t large_lvalue = L"rstuvw";
	std::initializer_list<wchar_t> init_list = { 'x', 'y', 'z', ',', '.', '/' };
	std::hash<test_string_t> hasher;

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
	test_mutation(L"abzcd", v.insert(v.begin() + 2, 'z'));
	test_mutation(L"abzzc", v.insert(v.begin() + 2, 2, 'z'));
	test_mutation(L"abfgh", v.insert(v.begin() + 2, std_string.begin(), std_string.end()));
	test_mutation(L"abABC", v.insert(v.begin() + 2, c_out_it{ 'A' }, c_out_it{ 'Z' }));
	test_mutation(L"axyz,", v.insert(1, init_list));
	test_mutation(L"aghbc", v.insert(1, std_string, 1, 2));
	test_mutation(L"", v.erase());
	test_mutation(L"ad", v.erase(1, 2));
	test_mutation(L"bcd", v.erase(v.begin()));
	test_mutation(L"cd", v.erase(v.begin(), v.begin() + 2));
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
	test_mutation(L"arstu", v.replace(1, 2, large_lvalue));
	test_mutation(L"arstu", v.replace(v.begin() + 1, v.begin() + 2, large_lvalue));
	test_mutation(L"astcd", v.replace(1, 1, large_lvalue, 1, 2));
	test_mutation(L"afghi", v.replace(v.begin() + 1, v.begin() + 2, std_string.begin(), std_string.end()));
	test_mutation(L"aABCD", v.replace(v.begin() + 1, v.begin() + 2, c_out_it{ 'A' }, c_out_it{ 'Z' }));
	test_mutation(L"aAcd", v.replace(v.begin() + 1, v.begin() + 2, c_out_it{ 'A' }, c_out_it{ 'B' }));
	test_mutation(L"a12cd", v.replace(1, 1, L"12", 2));
	test_mutation(L"a12cd", v.replace(v.begin() + 1, v.begin() + 2, L"12", 2));
	test_mutation(L"a12cd", v.replace(1, 1, L"12"));
	test_mutation(L"aad", v.replace(1, 2, 'a'));
	test_mutation(L"aad", v.replace(v.begin() + 1, v.begin() + 3, 'a'));
	test_mutation(L"axyz,", v.replace(v.begin() + 1, v.begin() + 3, init_list));
	test_mutation(L"aghcd", v.replace(1, 1, std_string, 1, 2));
	test_mutation(L"ab", v.resize(2));
	test_mutation(L"abcd1", v.resize(6, '1'));
	test_mutation(L"lmnop", { auto o{small_lvalue}; v.swap(o); });
	test_mutation(L"de", v = test_string_t(L"abcde").substr(3));
	test_mutation(L"d", v = test_string_t(L"abcde").substr(3, 1));
	test_mutation(L"d", v = test_string_t(L"abcde").substr(3, std::integral_constant<int, 1>{}));
	test_mutation(L"d", v = test_string_t(L"abcde").substr<1>(3, {}));
	test_mutation(L"rstuv", { wchar_t t[10]; large_lvalue.copy(t, 7); v = t; });
	test_mutation(L"rst", { wchar_t t[10]; large_lvalue.copy(t, 3); v = t; });
	test_mutation(L"stu", { wchar_t t[10]; large_lvalue.copy(t, 3, 1); v = t; });
	assert(small_lvalue.compare(large_lvalue) < 0);
	assert(small_lvalue.compare(1, 2, large_lvalue) < 0);
	assert(small_lvalue.compare(1, 2, large_lvalue, 1, 2) < 0);
	assert(small_lvalue.compare(L"lmnopq") < 0);
	assert(small_lvalue.compare(L"lmnop") == 0);
	assert(small_lvalue.compare(L"lmno") > 0);
	assert(small_lvalue.compare(L"k") > 0);
	assert(small_lvalue.compare(L"m") < 0);
	assert(small_lvalue.compare(1, 2, L"mno") < 0);
	assert(small_lvalue.compare(1, 2, L"mn") == 0);
	assert(small_lvalue.compare(1, 2, L"m") > 0);
	assert(small_lvalue.compare(1, 2, L"l") > 0);
	assert(small_lvalue.compare(1, 2, L"n") < 0);
	assert(small_lvalue.compare(1, 2, L"mno", 2) == 0);
	assert(small_lvalue.compare(1, 2, L"mmo", 2) > 0);
	assert(small_lvalue.compare(1, 2, L"moo", 2) < 0);
	assert(small_lvalue.compare(std_string) > 0);
	assert(small_lvalue.compare(1, 2, std_string, 1, 2) > 0);
	assert(small_lvalue.find(large_lvalue) == test_string_t::npos);
	assert(small_lvalue.find(large_lvalue, 1) == test_string_t::npos);
	assert(small_lvalue.find(L"nop", 1, 3) == 2);
	assert(small_lvalue.find(L"n", 1) == 2);
	assert(small_lvalue.find(L"n") == 2);
	assert(small_lvalue.find(L'n') == 2);
	assert(small_lvalue.find(L'n', 1) == 2);
	assert(small_lvalue.find(std_string) == test_string_t::npos);
	assert(small_lvalue.find(std_string, 1) == test_string_t::npos);
	assert(test_string_t(L"abcabc").find('b', 3) == 4);
	assert(small_lvalue.rfind(large_lvalue) == test_string_t::npos);
	assert(small_lvalue.rfind(large_lvalue, 5) == test_string_t::npos);
	assert(small_lvalue.rfind(L"nop", 5, 3) == 2);
	assert(small_lvalue.rfind(L"n", 5) == 2);
	assert(small_lvalue.rfind(L"n") == 2);
	assert(small_lvalue.rfind(L'n') == 2);
	assert(small_lvalue.rfind(L'n', 5) == 2);
	assert(small_lvalue.rfind(std_string) == test_string_t::npos);
	assert(small_lvalue.rfind(std_string, 5) == test_string_t::npos);
	assert(test_string_t(L"abcabc").rfind('b', 3) == 1);
	assert(small_lvalue.find_first_of(large_lvalue) == test_string_t::npos);
	assert(small_lvalue.find_first_of(large_lvalue, 1) == test_string_t::npos);
	assert(small_lvalue.find_first_of(L"nop", 1, 3) == 2);
	assert(small_lvalue.find_first_of(L"nop", 1) == 2);
	assert(small_lvalue.find_first_of(L"nop") == 2);
	assert(small_lvalue.find_first_of(L'n') == 2);
	assert(small_lvalue.find_first_of(std_string, 1) == test_string_t::npos);
	assert(small_lvalue.find_first_of(std_string) == test_string_t::npos);
	assert(small_lvalue.find_last_of(large_lvalue) == test_string_t::npos);
	assert(small_lvalue.find_last_of(large_lvalue, 1) == test_string_t::npos);
	assert(small_lvalue.find_last_of(L"nop", 2, 3) == 2);
	assert(small_lvalue.find_last_of(L"nop", 2) == 2);
	assert(small_lvalue.find_last_of(L"nop") == 4);
	assert(small_lvalue.find_last_of(L'n') == 2);
	assert(small_lvalue.find_last_of(std_string, 1) == test_string_t::npos);
	assert(small_lvalue.find_last_of(std_string) == test_string_t::npos);
	assert(small_lvalue.find_first_not_of(large_lvalue) == 0);
	assert(small_lvalue.find_first_not_of(large_lvalue, 1) == 1);
	assert(small_lvalue.find_first_not_of(L"nop", 1, 3) == 1);
	assert(small_lvalue.find_first_not_of(L"nop", 1) == 1);
	assert(small_lvalue.find_first_not_of(L"nop") == 0);
	assert(small_lvalue.find_first_not_of(L'n') == 0);
	assert(small_lvalue.find_first_not_of(std_string, 1) == 1);
	assert(small_lvalue.find_first_not_of(std_string) == 0);
	assert(small_lvalue.find_last_not_of(large_lvalue) == 5);
	assert(small_lvalue.find_last_not_of(large_lvalue, 1) == 1);
	assert(small_lvalue.find_last_not_of(L"nop", 2, 3) == 1);
	assert(small_lvalue.find_last_not_of(L"nop", 2) == 1);
	assert(small_lvalue.find_last_not_of(L"nop") == 5);
	assert(small_lvalue.find_last_not_of(L'n') == 5);
	assert(small_lvalue.find_last_not_of(std_string, 1) == 1);
	assert(small_lvalue.find_last_not_of(std_string) == 5);
	test_big_mutation(L"abcdlmn", v = v + small_lvalue);
	test_big_mutation(L"abcd123", v = v + L"123");
	test_big_mutation(L"123abcd", v = L"123" + v);
	test_big_mutation(L"abcd1", v = v + L'1');
	test_big_mutation(L"1abcd", v = L'1' + v);
	test_big_mutation(L"abcdfgh", v = v + std_string);
	test_big_mutation(L"fghijka", v = std_string + v);
	assert((small_lvalue == large_lvalue) == false);
	assert((small_lvalue != large_lvalue) == true);
	assert((small_lvalue < large_lvalue) == true);
	assert((small_lvalue <= large_lvalue) == true);
	assert((small_lvalue > large_lvalue) == false);
	assert((small_lvalue >= large_lvalue) == false);
	assert((small_lvalue == L"lmnop") == true);
	assert((small_lvalue == L"lmno") == false);
	assert((small_lvalue != L"lmnop") == false);
	assert((small_lvalue != L"lmno") == true);
	assert((small_lvalue < L"lmnop") == false);
	assert((small_lvalue < L"lmnoq") == true);
	assert((small_lvalue <= L"lmnop") == true);
	assert((small_lvalue <= L"lmnoo") == false);
	assert((small_lvalue > L"lmnop") == false);
	assert((small_lvalue > L"lmnoo") == true);
	assert((small_lvalue >= L"lmnop") == true);
	assert((small_lvalue >= L"lmnoq") == false);
	assert((small_lvalue == std_string) == false);
	assert((small_lvalue != std_string) == true);
	assert((small_lvalue < std_string) == false);
	assert((small_lvalue <= std_string) == false);
	assert((small_lvalue > std_string) == true);
	assert((small_lvalue >= std_string) == true);
	test_mutation(L"abd", erase(v, L'c'));
	auto pred = [](wchar_t wc) {return wc == L'c'; };
	test_mutation(L"abd", erase_if(v, pred));
	test_mutation(L"lmnop", istream(L"lmnopq") >> v);
	test_mutation(L"lmno", istream(L"lmno") >> v);
	test_mutation(L"abcd", { ostream_t out; out << v; v = out.str(); });
	test_mutation(L"lmnop", getline(istream(L"lmnopq"), v));
	test_mutation(L"lmno", getline(istream(L"lmno"), v));
	test_mutation(L"lmn", getline(istream(L"lmnopq"), v, L'o'));
	assert(stoi(char_test_string_t("1234")) == 1234);
	assert(stoi(test_string_t(L"1234")) == 1234);
	assert(stol(char_test_string_t("1234")) == 1234);
	assert(stol(test_string_t(L"1234")) == 1234);
	assert(stoll(char_test_string_t("1234")) == 1234ll);
	assert(stoll(test_string_t(L"1234")) == 1234ll);
	assert(stoul(char_test_string_t("1234")) == 1234ul);
	assert(stoul(test_string_t(L"1234")) == 1234ul);
	assert(stoull(char_test_string_t("1234")) == 1234ull);
	assert(stoull(test_string_t(L"1234")) == 1234ull);
	assert(stof(char_test_string_t("12.34")) == 12.34f);
	assert(stof(test_string_t(L"12.34")) == 12.34f);
	assert(stod(char_test_string_t("12.34")) == 12.34);
	assert(stod(test_string_t(L"12.34")) == 12.34);
	assert(stold(char_test_string_t("12.34")) == 12.34l);
	assert(stold(test_string_t(L"12.34")) == 12.34l);
	assert(mpd::to_small_string(INT_MIN) == std::to_string(INT_MIN));
	assert(mpd::to_small_string(LONG_MIN) == std::to_string(LONG_MIN));
	assert(mpd::to_small_string(LLONG_MIN) == std::to_string(LLONG_MIN));
	assert(mpd::to_small_string(UINT_MAX) == std::to_string(UINT_MAX));
	assert(mpd::to_small_string(ULONG_MAX) == std::to_string(ULONG_MAX));
	assert(mpd::to_small_string(ULLONG_MAX) == std::to_string(ULLONG_MAX));
	assert(mpd::to_small_string(ULLONG_MAX) == std::to_string(ULLONG_MAX));
	assert(mpd::to_small_string(-FLT_MIN) == std::to_string(-FLT_MIN));
	assert(mpd::to_small_string(-FLT_MAX) == std::to_string(-FLT_MAX));
	assert(mpd::to_small_string((double)-FLT_MIN) == std::to_string((double)-FLT_MIN));
	assert(mpd::to_small_string((double)-FLT_MAX) == std::to_string((double)-FLT_MAX));
	assert(mpd::to_small_string((long double)-FLT_MIN) == std::to_string((long double)-FLT_MIN));
	assert(mpd::to_small_string((long double)-FLT_MAX) == std::to_string((long double)-FLT_MAX));
	assert(mpd::to_small_wstring(INT_MIN) == std::to_wstring(INT_MIN));
	assert(mpd::to_small_wstring(LONG_MIN) == std::to_wstring(LONG_MIN));
	assert(mpd::to_small_wstring(LLONG_MIN) == std::to_wstring(LLONG_MIN));
	assert(mpd::to_small_wstring(UINT_MAX) == std::to_wstring(UINT_MAX));
	assert(mpd::to_small_wstring(ULONG_MAX) == std::to_wstring(ULONG_MAX));
	assert(mpd::to_small_wstring(ULLONG_MAX) == std::to_wstring(ULLONG_MAX));
	assert(mpd::to_small_wstring(ULLONG_MAX) == std::to_wstring(ULLONG_MAX));
	assert(mpd::to_small_wstring(-FLT_MIN) == std::to_wstring(-FLT_MIN));
	assert(mpd::to_small_wstring(-FLT_MAX) == std::to_wstring(-FLT_MAX));
	assert(mpd::to_small_wstring((double)-FLT_MIN) == std::to_wstring((double)-FLT_MIN));
	assert(mpd::to_small_wstring((double)-FLT_MAX) == std::to_wstring((double)-FLT_MAX));
	assert(mpd::to_small_wstring((long double)-FLT_MIN) == std::to_wstring((long double)-FLT_MIN));
	assert(mpd::to_small_wstring((long double)-FLT_MAX) == std::to_wstring((long double)-FLT_MAX));
	//using namespace mpd::literals;
	//assert("abcd"_smstr == "abcd");
	//assert(L"abcd"_smstr == L"abcd");
	assert(hasher(L"") == 0x9e3779b9ull);
	assert(hasher(L"ab") == 4133876243914441702ull);
	assert(hasher(L"ba") == 4389997032588314883ull);
	return 0;
}