#define MPD_SSTRING_OVERRUN_CHECKS
#include "strings/string_buffer.hpp"
#include <array>
#include <functional>
#include <iostream>
#include <sstream>
#include <utility>
#include <cwchar>

using w5string = mpd::array_wstring<5, mpd::overflow_behavior_t::truncate>;
using w7string = mpd::array_wstring<7, mpd::overflow_behavior_t::truncate, alignof(wchar_t)>;
using c5string = mpd::array_string<5, mpd::overflow_behavior_t::truncate>;
static_assert(alignof(w5string) == alignof(max_align_t), "w5string should be max aligned");
static_assert(alignof(w7string) == alignof(wchar_t), "w7string should be wchar_t aligned");
static_assert(alignof(c5string) == alignof(max_align_t), "c5string should be max aligned");

struct wchar_input_iter {
	using value_type = wchar_t;
	using difference_type = std::ptrdiff_t;
	using reference = wchar_t&;
	using pointer = wchar_t*;
	using iterator_category = std::input_iterator_tag;
	wchar_t c;
	wchar_input_iter operator++() { ++c; return *this; }
	wchar_input_iter operator++(int) { ++c; return *this; }
	wchar_t operator*() const { return c; }
	bool operator==(wchar_input_iter other) const { return c == other.c; }
	bool operator!=(wchar_input_iter other) const { return c != other.c; }
};

inline std::basic_istringstream<wchar_t> istream(const wchar_t* str) {
	return std::basic_istringstream<wchar_t>(str);
}

#define test_init() \
	w5_abcd = L"abcd"; \
	wstd_fghijk = L"fghijk"; \
	w5_lmnop = L"lmnop"; \
	w7_rstuvw = L"rstuvw"; \
	stdarray = {} \

#define test_is(op, expected) { \
	test_init(); \
	const auto& r = op; \
	if (r != expected) {\
		std::wcerr<< L"TEST FAILED: expected \"" << expected << L"\" but found \"" << r << L"\" during test of \"" L#op L"\"\n" << std::flush; \
		assert(false); \
	} \
}
#define test_is_without_ostream(op, expected) { \
	test_init(); \
	const auto& r = op; \
	if (r != expected) {\
		std::wcerr<< L"TEST FAILED: expected \"" << #expected << L"\" but found other during test of \"" << L#op << L"\"\n" << std::flush; \
		assert(false); \
	} \
}

#define test_abcd_after(op, expected) { \
	test_init(); \
	op; \
	if (w5_abcd != expected) {\
		std::wcerr<< L"TEST FAILED: expected \"" << expected << L"\" but found \"" << w5_abcd << L"\" during test of \"" << L#op << L"\"\n" << std::flush; \
		assert(false); \
	} \
}

#define test_expression_after(op, expression, expected) { \
	test_init(); \
	op; \
	const auto& r = expression; \
	if (r != expected) {\
		std::wcerr<< L"TEST FAILED: expected \"" << expected << L"\" but found \"" << r << L"\" during test of \"" << L#op << L"\", \"" << L#expression << "\"\n" << std::flush; \
		assert(false); \
	} \
}

void test_small_strings() {
	using ostream_t = std::basic_ostringstream<wchar_t>;
	w5string w5_abcd(L"abcd");
	std::wstring wstd_fghijk = L"fghijk";
	w5string w5_lmnop = L"lmnopq";
	w7string w7_rstuvw = L"rstuvw";
	std::initializer_list<wchar_t> il_xyzcds = { 'x', 'y', 'z', ',', '.', '/' };
	std::array<wchar_t, 7> stdarray = {};
	std::hash<w5string> hasher;
	test_is(w5string(), L"");
	test_is(w5string(4, L'a'), L"aaaa");
	test_is(w5string(7, L'a'), L"aaaaa");
	test_is(w5string(L"1234567"), L"12345");
	test_is(w5string(L"1234567", 7), L"12345");
	test_is(w5string(L"1234567"), L"12345");
	test_is(w5string(w5_lmnop), L"lmnop");
	test_is(w5string(std::move(w5_lmnop)), L"lmnop");
	test_is(w5string(il_xyzcds), L"xyz,.");
	test_is(w5string(wstd_fghijk), L"fghij");
	test_is(w5string(wstd_fghijk, 1, 3), L"ghi");
	test_is(w5string(wstd_fghijk, 5, 5), L"k");
	test_is(w5string(w7string(L"123")), L"123");
	test_is(w5string(w7_rstuvw), L"rstuv");
	test_is(w5string(w7_rstuvw, 1), L"stuvw");
	test_is(w5string(w7_rstuvw, 1, 3), L"stu");
	test_is(w5string(w7_rstuvw, 5, 5), L"w");
	test_is(w5string(wchar_input_iter{ 'A' }, wchar_input_iter{ 'Z' }), L"ABCDE");
	test_is(w5_abcd = w7_rstuvw, L"rstuv");
	test_is(w5_abcd = w5_lmnop, L"lmnop");
	test_is(w5_abcd = std::move(w5_lmnop), L"lmnop");
	test_is(w5_abcd = L"123", L"123");
	test_is(w5_abcd = L'a', L"a");
	test_is(w5_abcd = il_xyzcds, L"xyz,.");
	test_is(w5_abcd = wstd_fghijk, L"fghij");
	test_is(w5_abcd.assign(4, L'a'), L"aaaa");
	test_is(w5_abcd.assign(7, L'a'), L"aaaaa");
	test_is(w5_abcd.assign(w7_rstuvw), L"rstuv");
	test_is(w5_abcd.assign(w7_rstuvw, 1, 3), L"stu");
	test_is(w5_abcd.assign(w7_rstuvw, 5, 5), L"w");
	test_is(w5_abcd.assign(w5_lmnop), L"lmnop");
	test_is(w5_abcd.assign(std::move(w5_lmnop)), L"lmnop");
	test_is(w5_abcd.assign(L"1234567"), L"12345");
	test_is(w5_abcd.assign(L"1234567", 2), L"12");
	test_is(w5_abcd.assign(il_xyzcds), L"xyz,.");
	test_is(w5_abcd.assign(wstd_fghijk), L"fghij");
	test_is(w5_abcd.assign(wchar_input_iter{ 'A' }, wchar_input_iter{ 'Z' }), L"ABCDE");
	test_is_without_ostream(w5_lmnop.get_allocator(), std::allocator<char>{});
	test_is(w5_lmnop.at(2), 'n');
	test_is(w5_lmnop[2], 'n');
	test_is(*w5_lmnop.begin(), 'l');
	test_is(*w5_lmnop.cbegin(), 'l');
	test_is(*(w5_lmnop.end()-1), 'p');
	test_is(*(w5_lmnop.cend() - 1), 'p');
	test_is(*w5_lmnop.rbegin(), 'p');
	test_is(*w5_lmnop.crbegin(), 'p');
	test_is(*--w5_lmnop.rend(), 'l');
	test_is(*--w5_lmnop.crend(), 'l');
	test_is(w5string(L"a").empty(), false);
	test_is(w5string(L"").empty(), true);
	test_is(w5string(L"").size(), 0);
	test_is(w5_lmnop.size(), 5);
	test_is(w5string(L"").length(), 0);
	test_is(w5_lmnop.length(), 5);
	test_is(w5string().capacity(), 5);
	test_expression_after(w5_abcd.reserve(4), w5_abcd.capacity(), 5);
	test_expression_after(w5_abcd.shrink_to_fit(), w5_abcd.capacity(), 5);
	test_expression_after(w5_abcd.clear(), w5_abcd.size(), 0);
	test_is(w5_abcd.insert(1, 2, 'z'), L"azzbc");
	test_is(w5_abcd.insert(1, L"zz"), L"azzbc");
	test_is(w5_abcd.insert(1, L"zz", 1), L"azbcd");
	test_is(w5_abcd.insert(1, w7_rstuvw), L"arstu");
	test_is(w5_abcd.insert(1, w7_rstuvw, 2, 1), L"atbcd");
	test_abcd_after(w5_abcd.insert(w5_abcd.begin() + 2, 'z'), L"abzcd");
	test_abcd_after(w5_abcd.insert(w5_abcd.begin() + 2, 2, 'z'), L"abzzc");
	test_abcd_after(w5_abcd.insert(w5_abcd.begin() + 2, wstd_fghijk.begin(), wstd_fghijk.end()), L"abfgh");
	test_abcd_after(w5_abcd.insert(w5_abcd.begin() + 2, wchar_input_iter{ 'A' }, wchar_input_iter{ 'Z' }), L"abABC");
	test_abcd_after(w5_abcd.insert(1, il_xyzcds), L"axyz,");
	test_abcd_after(w5_abcd.insert(1, wstd_fghijk, 1, 2), L"aghbc");
	test_abcd_after(w5_abcd.erase(), L"");
	test_abcd_after(w5_abcd.erase(1, 2), L"ad");
	test_abcd_after(w5_abcd.erase(w5_abcd.begin()), L"bcd");
	test_abcd_after(w5_abcd.erase(w5_abcd.begin(), w5_abcd.begin() + 2), L"cd");
	test_abcd_after(w5_abcd.push_back('z'), L"abcdz");
	test_abcd_after(w5_abcd.pop_back(), L"abc");
	test_is(w5_abcd.append(2, 'z'), L"abcdz");
	test_is(w5_abcd.append(w7_rstuvw), L"abcdr");
	test_is(w5_abcd.append(w7_rstuvw, 1, 2), L"abcds");
	test_is(w5_abcd.append(L"123", 3), L"abcd1");
	test_is(w5_abcd.append(L"123"), L"abcd1");
	test_is(w5_abcd.append(wchar_input_iter{ 'A' }, wchar_input_iter{ 'Z' }), L"abcdA");
	test_is(w5_abcd.append(il_xyzcds), L"abcdx");
	test_is(w5_abcd.append(wstd_fghijk, 1, 2), L"abcdg");
	test_is(w5_abcd += w7_rstuvw, L"abcdr");
	test_is(w5_abcd += L'a', L"abcda");
	test_is(w5_abcd += L"123", L"abcd1");
	test_is(w5_abcd += il_xyzcds, L"abcdx");
	test_is(w5_abcd += wstd_fghijk, L"abcdf");
	test_is(w5_abcd.replace(1, 2, w7_rstuvw), L"arstu");
	test_is(w5_abcd.replace(w5_abcd.begin() + 1, w5_abcd.begin() + 2, w7_rstuvw), L"arstu");
	test_is(w5_abcd.replace(1, 1, w7_rstuvw, 1, 2), L"astcd");
	test_is(w5_abcd.replace(w5_abcd.begin() + 1, w5_abcd.begin() + 2, wstd_fghijk.begin(), wstd_fghijk.end()), L"afghi");
	test_is(w5_abcd.replace(w5_abcd.begin() + 1, w5_abcd.begin() + 2, wchar_input_iter{ 'A' }, wchar_input_iter{ 'Z' }), L"aABCD");
	test_is(w5_abcd.replace(w5_abcd.begin() + 1, w5_abcd.begin() + 2, wchar_input_iter{ 'A' }, wchar_input_iter{ 'B' }), L"aAcd");
	test_is(w5_abcd.replace(1, 1, L"12", 2), L"a12cd");
	test_is(w5_abcd.replace(w5_abcd.begin() + 1, w5_abcd.begin() + 2, L"12", 2), L"a12cd");
	test_is(w5_abcd.replace(1, 1, L"12"), L"a12cd");
	test_is(w5_abcd.replace(1, 2, 1, 'a'), L"aad");
	test_is(w5_abcd.replace(w5_abcd.begin() + 1, w5_abcd.begin() + 3, 1, 'a'), L"aad");
	test_is(w5_abcd.replace(w5_abcd.begin() + 1, w5_abcd.begin() + 3, il_xyzcds), L"axyz,");
	test_is(w5_abcd.replace(1, 1, wstd_fghijk, 1, 2), L"aghcd");
	test_abcd_after(w5_abcd.resize(2), L"ab");
	test_abcd_after(w5_abcd.resize(6, '1'), L"abcd1");
	test_abcd_after(w5_abcd.swap(w5_lmnop), L"lmnop");
	test_is(w5_abcd = w5string(L"abcde").substr(3), L"de");
	test_is(w5_abcd = w5string(L"abcde").substr(3, 1), L"d");
	test_is(w5_abcd = w5string(L"abcde").substr(3, std::integral_constant<int, 1>{}), L"d");
	test_is(w5_abcd = w5string(L"abcde").substr<1>(3), L"d");
	test_abcd_after({w7_rstuvw.copy(stdarray.data(), 7); w5_abcd = stdarray.data(); }, L"rstuv");
	test_abcd_after({w7_rstuvw.copy(stdarray.data(), 3); w5_abcd = stdarray.data();}, L"rst");
	test_abcd_after({w7_rstuvw.copy(stdarray.data(), 3, 1); w5_abcd = stdarray.data();; }, L"stu");
	test_is(w5_lmnop.compare(w7_rstuvw), -1);
	test_is(w5_lmnop.compare(1, 2, w7_rstuvw), -1);
	test_is(w5_lmnop.compare(1, 2, w7_rstuvw, 1, 2), -1);
	test_is(w5_lmnop.compare(L"lmnopq"), -1);
	test_is(w5_lmnop.compare(L"lmnop"), 0);
	test_is(w5_lmnop.compare(L"lmno"), 1);
	test_is(w5_lmnop.compare(L"k"), 1);
	test_is(w5_lmnop.compare(L"m"), -1);
	test_is(w5_lmnop.compare(1, 2, L"mno"), -1);
	test_is(w5_lmnop.compare(1, 2, L"mn"), 0);
	test_is(w5_lmnop.compare(1, 2, L"m"), 1);
	test_is(w5_lmnop.compare(1, 2, L"l"), 1);
	test_is(w5_lmnop.compare(1, 2, L"n"), -1);
	test_is(w5_lmnop.compare(1, 2, L"mno", 2), 0);
	test_is(w5_lmnop.compare(1, 2, L"mmo", 2), 1);
	test_is(w5_lmnop.compare(1, 2, L"moo", 2), -1);
	test_is(w5_lmnop.compare(wstd_fghijk), 1);
	test_is(w5_lmnop.compare(1, 2, wstd_fghijk, 1, 2), 1);
	test_is(w5_lmnop.find(w7_rstuvw), w5string::npos);
	test_is(w5_lmnop.find(w7_rstuvw, 1), w5string::npos);
	test_is(w5_lmnop.find(L"nop", 1, 3), 2);
	test_is(w5_lmnop.find(L"n", 1), 2);
	test_is(w5_lmnop.find(L"n"), 2);
	test_is(w5_lmnop.find(L'n'), 2);
	test_is(w5_lmnop.find(L'n', 1), 2);
	test_is(w5_lmnop.find(wstd_fghijk), w5string::npos);
	test_is(w5_lmnop.find(wstd_fghijk, 1), w5string::npos);
	test_is(w5string(L"abcabc").find('b', 3), 4);
	test_is(w5_lmnop.rfind(w7_rstuvw), w5string::npos);
	test_is(w5_lmnop.rfind(w7_rstuvw, 5), w5string::npos);
	test_is(w5_lmnop.rfind(L"nop", 5, 3), 2);
	test_is(w5_lmnop.rfind(L"n", 5), 2);
	test_is(w5_lmnop.rfind(L"n"), 2);
	test_is(w5_lmnop.rfind(L'n'), 2);
	test_is(w5_lmnop.rfind(L'n', 5), 2);
	test_is(w5_lmnop.rfind(wstd_fghijk), w5string::npos);
	test_is(w5_lmnop.rfind(wstd_fghijk, 5), w5string::npos);
	test_is(w5string(L"abcabc").rfind('b', 3), 1);
	test_is(w5_lmnop.find_first_of(w7_rstuvw), w5string::npos);
	test_is(w5_lmnop.find_first_of(w7_rstuvw, 1), w5string::npos);
	test_is(w5_lmnop.find_first_of(L"nop", 1, 3), 2);
	test_is(w5_lmnop.find_first_of(L"nop", 1), 2);
	test_is(w5_lmnop.find_first_of(L"nop"), 2);
	test_is(w5_lmnop.find_first_of(L'n'), 2);
	test_is(w5_lmnop.find_first_of(wstd_fghijk, 1), w5string::npos);
	test_is(w5_lmnop.find_first_of(wstd_fghijk), w5string::npos);
	/*
	test_is(w5_lmnop.find_last_of(w7_rstuvw), w5string::npos);
	test_is(w5_lmnop.find_last_of(w7_rstuvw, 1), w5string::npos);
	test_is(w5_lmnop.find_last_of(L"nop", 3, 2), 2);
	test_is(w5_lmnop.find_last_of(L"nop", 3), 2);
	test_is(w5_lmnop.find_last_of(L"nop"), 2);
	test_is(w5_lmnop.find_last_of(L'n'), 2);
	test_is(w5_lmnop.find_last_of(wstd_fghijk, 1), w5string::npos);
	test_is(w5_lmnop.find_last_of(wstd_fghijk), w5string::npos);
	*/
	test_is(w5_lmnop.find_first_not_of(w7_rstuvw), 0);
	test_is(w5_lmnop.find_first_not_of(w7_rstuvw, 1), 1);
	test_is(w5_lmnop.find_first_not_of(L"nop", 1, 3), 1);
	test_is(w5_lmnop.find_first_not_of(L"nop", 1), 1);
	test_is(w5_lmnop.find_first_not_of(L"nop"), 0);
	test_is(w5_lmnop.find_first_not_of(L'n'), 0);
	test_is(w5_lmnop.find_first_not_of(wstd_fghijk, 1), 1);
	/*
	test_is(w5_lmnop.find_first_not_of(wstd_fghijk), 0);
	test_is(w5_lmnop.find_last_not_of(w7_rstuvw), 5);
	test_is(w5_lmnop.find_last_not_of(w7_rstuvw, 1), 1);
	test_is(w5_lmnop.find_last_not_of(L"nop", 2, 3), 1);
	test_is(w5_lmnop.find_last_not_of(L"nop", 2), 1);
	test_is(w5_lmnop.find_last_not_of(L"nop"), 5);
	test_is(w5_lmnop.find_last_not_of(L'n'), 5);
	test_is(w5_lmnop.find_last_not_of(wstd_fghijk, 1), 1);
	test_is(w5_lmnop.find_last_not_of(wstd_fghijk), 5);
	*/
	test_is(w5_abcd = w5_abcd + w5_lmnop, L"abcdl");
	test_is(w5_abcd = w5_abcd + L"123", L"abcd1");
	test_is(w5_abcd = L"123" + w5_abcd, L"123ab");
	test_is(w5_abcd = w5_abcd + L'1', L"abcd1");
	test_is(w5_abcd = L'1' + w5_abcd, L"1abcd");
	test_is(w5_abcd = w5_abcd + wstd_fghijk, L"abcdf");
	test_is(w5_abcd = wstd_fghijk + w5_abcd, L"fghij");
	test_is((w5_lmnop == w7_rstuvw), false);
	test_is((w5_lmnop != w7_rstuvw), true);
	test_is((w5_lmnop < w7_rstuvw), true);
	test_is((w5_lmnop <= w7_rstuvw), true);
	test_is((w5_lmnop > w7_rstuvw), false);
	test_is((w5_lmnop >= w7_rstuvw), false);
	test_is((w5_lmnop == L"lmnop"), true);
	test_is((w5_lmnop == L"lmno"), false);
	test_is((w5_lmnop == L"labc"), false);
	test_is((w5_lmnop != L"lmnop"), false);
	test_is((w5_lmnop != L"lmno"), true);
	test_is((w5_lmnop != L"labc"), true);
	test_is((w5_lmnop < L"labc"), false);
	test_is((w5_lmnop < L"lmnop"), false);
	test_is((w5_lmnop < L"lzyx"), true);
	test_is((w5_lmnop <= L"labc"), false);
	test_is((w5_lmnop <= L"lmnop"), true);
	test_is((w5_lmnop <= L"lzyx"), true);
	test_is((w5_lmnop > L"labc"), true);
	test_is((w5_lmnop > L"lmnop"), false);
	test_is((w5_lmnop > L"lzyx"), false);
	test_is((w5_lmnop >= L"labc"), true);
	test_is((w5_lmnop >= L"lmnop"), true);
	test_is((w5_lmnop >= L"lzyx"), false);
	test_is((w5_lmnop == wstd_fghijk), false);
	test_is((w5_lmnop != wstd_fghijk), true);
	test_is((w5_lmnop < wstd_fghijk), false);
	test_is((w5_lmnop <= wstd_fghijk), false);
	test_is((w5_lmnop > wstd_fghijk), true);
	test_is((w5_lmnop >= wstd_fghijk), true);
	test_abcd_after(erase(w5_abcd, L'c'), L"abd");
	auto pred = [](wchar_t wc) {return wc == L'c'; };
	test_abcd_after(erase_if(w5_abcd, pred), L"abd");
	test_abcd_after(istream(L"lmnopq") >> w5_abcd, L"lmnop");
	test_abcd_after(istream(L"lmno") >> w5_abcd, L"lmno");
	test_abcd_after({ ostream_t out; out << w5_abcd; w5_abcd = out.str(); }, L"abcd");
	test_abcd_after({ std::basic_istringstream<wchar_t> s(L"lmnopq"); getline(s, w5_abcd); }, L"lmnop");
	test_abcd_after({ std::basic_istringstream<wchar_t> s(L"lmno"); getline(s, w5_abcd); }, L"lmno");
	test_abcd_after({ std::basic_istringstream<wchar_t> s(L"lmnopq"); getline(s, w5_abcd, L'o'); }, L"lmn");
	test_is(stoi(c5string("1234")), 1234);
	test_is(stoi(w5string(L"1234")), 1234);
	test_is(stol(c5string("1234")), 1234);
	test_is(stol(w5string(L"1234")), 1234);
	test_is(stoll(c5string("1234")), 1234ll);
	test_is(stoll(w5string(L"1234")), 1234ll);
	test_is(stoul(c5string("1234")), 1234ul);
	test_is(stoul(w5string(L"1234")), 1234ul);
	test_is(stoull(c5string("1234")), 1234ull);
	test_is(stoull(w5string(L"1234")), 1234ull);
	test_is(stof(c5string("12.34")), 12.34f);
	test_is(stof(w5string(L"12.34")), 12.34f);
	test_is(stod(c5string("12.34")), 12.34);
	test_is(stod(w5string(L"12.34")), 12.34);
	test_is(stold(c5string("12.34")), 12.34l);
	test_is(stold(w5string(L"12.34")), 12.34l);
	test_is(hasher(L""), 2220239839302836627ull);
	test_is(hasher(L"ab"), 8270537381191443548ull);
	test_is(hasher(L"ba"), 1250039036060834774ull);
}