
#include <cwchar>
#include <functional>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
#include "containers/front_buffer.hpp"

template<int unique>
class testing {
	static unsigned count_;
	wchar_t value_;
	const void* self;
public:
	testing() : value_('A' + count_), self(this) { ++count_; }
	testing(wchar_t value) : value_(value), self(this) { ++count_; }
	testing(const testing& rhs) : value_(rhs.value_), self(this) { assert(rhs.self == &rhs); ++count_; 	}
	template<int other>
	testing(const testing<other>& rhs) : value_(rhs.value_), self(this) { assert(rhs.self == &rhs); ++count_; }
	~testing() {
		assert(self == this);
		value_ = 0;
		self = nullptr;
		--count_;
	}
	testing& operator=(const testing& rhs) {
		assert(self == this);
		assert(rhs.self == &rhs);
		value_ = rhs.value_;
		return *this;
	}
	template<int other>
	testing& operator=(const testing<other>& rhs) {
		assert(self == this);
		assert(rhs.self == &rhs);
		value_ = rhs.value_;
		return *this;
	}
	wchar_t get() const { return value_; }
	static void reset_count(int count = 0) { count_ = count; }
	static int check_count() { return count_; }
	static void assert_count() { assert(count_==0); }
};
template<int unique>
std::wostream& operator<<(std::wostream& out, const testing<unique>& i) {
	if (std::isprint(i.get()))
		return out << i.get();
	else
		return out << "0x" << std::hex << +i.get();
}

template<int unique1, int unique2>
bool operator==(const testing<unique1>& lhs, const testing<unique2>& rhs) { return lhs.get() == rhs.get(); }
template<int unique1, int unique2>
bool operator!=(const testing<unique1>& lhs, const testing<unique2>& rhs) { return lhs.get() != rhs.get(); }
template<int unique1, int unique2>
bool operator<(const testing<unique1>& lhs, const testing<unique2>& rhs) { return lhs.get() < rhs.get(); }
template<int unique1, int unique2>
bool operator<=(const testing<unique1>& lhs, const testing<unique2>& rhs) { return lhs.get() <= rhs.get(); }
template<int unique1, int unique2>
bool operator>(const testing<unique1>& lhs, const testing<unique2>& rhs) { return lhs.get() > rhs.get(); }
template<int unique1, int unique2>
bool operator>=(const testing<unique1>& lhs, const testing<unique2>& rhs) { return lhs.get() >= rhs.get(); }
template<int unique1>
bool operator==(const testing<unique1>& lhs, wchar_t rhs) { return lhs.get() == rhs; }
template<int unique1>
bool operator!=(const testing<unique1>& lhs, wchar_t rhs) { return lhs.get() != rhs; }
template<int unique1>
bool operator<(const testing<unique1>& lhs, wchar_t rhs) { return lhs.get() < rhs; }
template<int unique1>
bool operator<=(const testing<unique1>& lhs, wchar_t rhs) { return lhs.get() <= rhs; }
template<int unique1>
bool operator>(const testing<unique1>& lhs, wchar_t rhs) { return lhs.get() > rhs; }
template<int unique1>
bool operator>=(const testing<unique1>& lhs, wchar_t rhs) { return lhs.get() >= rhs; }
namespace std {
	template<int unique>
	struct hash<testing<unique>> {
		std::size_t operator()(const testing<unique>& val) const noexcept {
			std::hash<wchar_t> subhasher;
			return subhasher(val.get());
		}
	};
}

template<int unique>
unsigned testing<unique>::count_ = 0;

using buffer0_5 = mpd::array_buffer<testing<0>, 5, mpd::overflow_behavior_t::truncate>;
using buffer0_7 = mpd::array_buffer<testing<0>, 7, mpd::overflow_behavior_t::truncate>;
using buffer1_5 = mpd::array_buffer<testing<1>, 5, mpd::overflow_behavior_t::truncate>;

struct wchar_input_iter {
	using value_type = testing<0>;
	using difference_type = std::ptrdiff_t;
	using reference = testing<0>&;
	using pointer = testing<0>*;
	using iterator_category = std::input_iterator_tag;
	wchar_t c;
	wchar_input_iter operator++() { ++c; return *this; }
	wchar_input_iter operator++(int) { ++c; return *this; }
	testing<0> operator*() const { return testing<0>(c); }
	bool operator==(wchar_input_iter other) const { return c == other.c; }
	bool operator!=(wchar_input_iter other) const { return c != other.c; }
};

inline std::basic_istringstream<wchar_t> istream(const wchar_t* vec) {
	return std::basic_istringstream<wchar_t>(vec);
}

template<class Range>
struct range_printer {
	const Range* r;
};
template<class Range>
std::wostream& operator<<(std::wostream& out, const range_printer<Range>& p) {
	out << L'{';
	for (const auto& i : *(p.r))
		out << i << L", ";
	return out << L'}';
}
template<unsigned N>
std::wostream& operator<<(std::wostream& out, const range_printer<wchar_t[N]>& p) {
	return out << L'\"' << *(p.r) << L'\"';
}
template <class Range, class It = decltype(std::begin(std::declval<const Range&>()))>
range_printer<Range> print(const Range& r) { return {&r}; }

void test_init(buffer0_5& b05_abcd, std::vector<testing<0>>& std0_fghijk, buffer0_5& b05_lmnop, buffer0_7& b07_rstuvw) {
	b05_abcd = {{'a'}, {'b'}, {'c'}, {'d'}};
	std0_fghijk = {{'f'}, {'g'}, {'h'}, {'i'}, {'j'}, {'k'}};
	b05_lmnop = {{'l'}, {'m'}, {'n'}, {'o'}, {'p'}, {'q'}};
	b07_rstuvw = {{'r'}, {'s'}, {'t'}, {'u'}, {'v'}, {'w'}};
}

#define test_is(op, expected) { \
	test_init(b05_abcd, std0_fghijk, b05_lmnop, b07_rstuvw); \
	const auto& r = op; \
	if (r != expected) {\
		std::wcerr<< L"TEST FAILED: expected " << expected << L" but found \"" << r << L" during test of \"" L#op L"\"\n" << std::flush; \
		assert(false); \
	} \
}
#define test_range_is(op, expected) { \
	test_init(b05_abcd, std0_fghijk, b05_lmnop, b07_rstuvw); \
	const auto& r = op; \
	if (std::size(r) != (std::size(expected)-1) || !std::equal(r.cbegin(), r.cend(), std::cbegin(expected))) { \
		std::wcerr<< L"TEST FAILED: expected " << print(expected) << L" but found " << print(r) << L" during test of \"" L#op L"\"\n" << std::flush; \
		assert(false); \
	} \
}
#define test_is_without_ostream(op, expected) { \
	test_init(b05_abcd, std0_fghijk, b05_lmnop, b07_rstuvw); \
	const auto& r = op; \
	if (r != expected) {\
		std::wcerr<< L"TEST FAILED: expected " << #expected << L" but found other during test of \"" << L#op << L"\"\n" << std::flush; \
		assert(false); \
	} \
}

#define test_abcd_after(op, expected) { \
	test_init(b05_abcd, std0_fghijk, b05_lmnop, b07_rstuvw); \
	op; \
	if (std::size(b05_abcd) != (std::size(expected)-1) || !std::equal(b05_abcd.cbegin(), b05_abcd.cend(), std::cbegin(expected))) { \
		std::wcerr<< L"TEST FAILED: expected " << print(expected) << L" but found " << print(b05_abcd) << L" during test of \"" << L#op << L"\"\n" << std::flush; \
		assert(false); \
	} \
}

#define test_expression_after(op, expression, expected) { \
	test_init(b05_abcd, std0_fghijk, b05_lmnop, b07_rstuvw); \
	op; \
	const auto& r = expression; \
	if (r != expected) {\
		std::wcerr<< L"TEST FAILED: expected " << expected << L" but found " << r << L" during test of \"" << L#op << L"\", (" << L#expression << ")\n" << std::flush; \
		assert(false); \
	} \
}

void test_small_vectors() {
	buffer0_5 b05_abcd({{'a'}, {'b'}, {'c'}, {'d'}});
	std::vector<testing<0>> std0_fghijk{ {'f'}, {'g'}, {'h'}, {'i'}, {'j'}, {'k'} };
	buffer0_5 b05_lmnop { {'l'}, {'m'}, {'n'}, {'o'}, {'p'}, {'q'} };
	buffer0_7 b07_rstuvw { {'r'}, {'s'}, {'t'}, {'u'}, {'v' }, { 'w' }};
	std::initializer_list<testing<0>> ilist_xyzcps { {'x'}, {'y'}, {'z'}, {','}, {'.'}, {'/'} };
	std::hash<buffer0_5> hasher;

	test_range_is(buffer0_5{}, L"");
	//test_range_is(4, mpd::default_not_value_construct{}, L"aaaa");
	test_range_is(buffer0_5(std::initializer_list<testing<0>>{ {'1'}, {'2'}, {'3'}, {'4'}, {'5' }, { '6' }, { '7' } }), L"12345");
	test_range_is(buffer0_5(b05_lmnop), L"lmnop");
	test_range_is(buffer0_5(std::move(b05_lmnop)), L"lmnop");
	test_range_is(buffer0_5(ilist_xyzcps), L"xyz,.");
	test_range_is(buffer0_5(std0_fghijk), L"fghij");
	test_range_is(buffer0_5(buffer0_7{ {'1'},{'2'},{'3' }}), L"123");
	test_range_is(buffer0_5(b07_rstuvw), L"rstuv");
	test_range_is(buffer0_5(wchar_input_iter{ 'A' }, wchar_input_iter{ 'Z' }), L"ABCDE");
	test_range_is(b05_abcd = b07_rstuvw, L"rstuv");
	test_range_is(b05_abcd = b05_lmnop, L"lmnop");
	test_range_is(b05_abcd = std::move(b05_lmnop), L"lmnop");
	test_range_is((b05_abcd = { {'1'}, {'2'}, {'3'} }), L"123");
	test_range_is(b05_abcd = ilist_xyzcps, L"xyz,.");
	test_range_is(b05_abcd = std0_fghijk, L"fghij");
	test_abcd_after(b05_abcd.assign(4, L'a'), L"aaaa");
	test_abcd_after(b05_abcd.assign(7, L'a'), L"aaaaa");
	test_abcd_after(b05_abcd.assign({ {'1'},{'2'},{'3'},{'4'},{'5'},{'6'},{'7'} }), L"12345");
	test_abcd_after(b05_abcd.assign(ilist_xyzcps), L"xyz,.");
	test_abcd_after(b05_abcd.assign(wchar_input_iter{ 'A' }, wchar_input_iter{ 'Z' }), L"ABCDE");
	test_is_without_ostream(b05_lmnop.get_allocator(), std::allocator<char>{});
	test_is(b05_lmnop.at(2), 'n');
	test_is(b05_lmnop[2], 'n');
	test_is(*b05_lmnop.begin(), 'l');
	test_is(*b05_lmnop.cbegin(), 'l');
	auto it1 = b05_lmnop.end();
	test_is(*--it1, 'p');
	auto it2 = b05_lmnop.cend();
	test_is(*--it2, 'p');
	test_is(*b05_lmnop.rbegin(), 'p');
	test_is(*b05_lmnop.crbegin(), 'p');
	test_is(*--b05_lmnop.rend(), 'l');
	test_is(*--b05_lmnop.crend(), 'l');
	test_is(buffer0_5( 'a' ).empty(), false);
	test_is(buffer0_5().empty(), true);
	test_is(buffer0_5().size(), 0);
	test_is(b05_lmnop.size(), 5);
	test_is(buffer0_5().capacity(), 5);
	test_expression_after(b05_abcd.reserve(4), b05_abcd.capacity(), 5);
	test_expression_after(b05_abcd.shrink_to_fit(), b05_abcd.capacity(), 5);
	test_expression_after(b05_abcd.operator=({{'t'}, {'e'}, {'s'}, {'t'}}).clear(), b05_abcd.size(), 0);
	test_abcd_after(b05_abcd.insert(b05_abcd.begin()+1, 2, {'z'}), L"azzbc");
	test_abcd_after(b05_abcd.insert(b05_abcd.begin() + 1, {{ 'z' }, { 'z' }}), L"azzbc");
	test_abcd_after(b05_abcd.insert(b05_abcd.begin() + 2, { 'z' }), L"abzcd");
	test_abcd_after(b05_abcd.insert(b05_abcd.begin() + 2, 2, { 'z' }), L"abzzc");
	test_abcd_after(b05_abcd.insert(b05_abcd.begin() + 2, std0_fghijk.begin(), std0_fghijk.end()), L"abfgh");
	test_abcd_after(b05_abcd.insert(b05_abcd.begin() + 2, wchar_input_iter{ 'A' }, wchar_input_iter{ 'Z' }), L"abABC");
	test_abcd_after(b05_abcd.insert(b05_abcd.begin() + 1, ilist_xyzcps), L"axyz,");
	test_abcd_after(b05_abcd.erase(b05_abcd.begin() + 1, b05_abcd.begin()+2), L"acd");
	test_abcd_after(b05_abcd.erase(b05_abcd.begin()), L"bcd");
	test_abcd_after(b05_abcd.erase(b05_abcd.begin(), b05_abcd.begin() + 2), L"cd");
	test_abcd_after(b05_abcd.push_back('z'), L"abcdz");
	test_abcd_after(b05_abcd.emplace_back('z'), L"abcdz");
	test_abcd_after(b05_abcd.pop_back(), L"abc");
	test_abcd_after(b05_abcd.resize(2), L"ab");
	test_abcd_after(b05_abcd.resize(6, '1'), L"abcd1");
	//test_range_is(b05_abcd.resize(6, mpd::default_not_value_construct{}), L"abcd1");
	//test_range_is({ auto o{b05_lmnop}; b05_abcd.swap(o); }, L"lmnop");
	test_is((b05_lmnop == b07_rstuvw), false);
	test_is((b05_lmnop != b07_rstuvw), true);
	test_is((b05_lmnop < b07_rstuvw), true);
	test_is((b05_lmnop <= b07_rstuvw), true);
	test_is((b05_lmnop > b07_rstuvw), false);
	test_is((b05_lmnop >= b07_rstuvw), false);
	//test_is((b05_lmnop == std0_fghijk), false);
	//test_is((b05_lmnop != std0_fghijk), true);
	//test_is((b05_lmnop < std0_fghijk), false);
	//test_is((b05_lmnop <= std0_fghijk), false);
	//test_is((b05_lmnop > std0_fghijk), true);
	//test_is((b05_lmnop >= std0_fghijk), true);
	test_abcd_after(erase(b05_abcd, L'c'), L"abd");
	auto pred = [](const testing<0>& b05_abcd) {return b05_abcd == L'c'; };
	test_abcd_after(erase_if(b05_abcd, pred), L"abd");
	test_is(hasher(buffer0_5{}), 16228791721261791980ull);
	test_is(hasher(buffer0_5{ {'a'},{'b'} }), 13301184043639500456ull);
	test_is(hasher(buffer0_5{ {'b'},{'a'} }), 9275817057764757695ull);
}
