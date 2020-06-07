#define MPD_SVEVTOR_OVERRUN_CHECKS
#include "../SmallContainers/small_vector.h"
#include <functional>
#include <iostream>
#include <sstream>
#include <utility>
#include <cwchar>

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

using test_vector_t = mpd::svector_local<testing<0>, 5, mpd::overflow_behavior_t::silently_truncate>;
using bigger_vector_t = mpd::svector_local<testing<0>, 7, mpd::overflow_behavior_t::silently_truncate>;
using alt_test_string_t = mpd::svector_local<testing<1>, 5, mpd::overflow_behavior_t::silently_truncate>;

struct c_in_it {
	using value_type = testing<0>;
	using difference_type = std::ptrdiff_t;
	using reference = testing<0>&;
	using pointer = testing<0>*;
	using iterator_category = std::input_iterator_tag;
	wchar_t c;
	c_in_it operator++() { ++c; return *this; }
	c_in_it operator++(int) { ++c; return *this; }
	testing<0> operator*() const { return testing<0>(c); }
	bool operator==(c_in_it other) const { return c == other.c; }
	bool operator!=(c_in_it other) const { return c != other.c; }
};

template<class...Ts>
void test_vec_ctor(const wchar_t* expected_value, Ts&&... vals) {
	test_vector_t vec(std::forward<Ts>(vals)...);
	std::size_t found_size = vec.size();
	auto found_value = vec.data();
	assert(found_size == wcslen(expected_value));
	assert(std::equal(vec.cbegin(), vec.cend(), expected_value));
}

template<class F>
void _test_vec_mutation(const wchar_t* expected_value, F func) {
	test_vector_t vec({ {'a'},{'b'}, {'c'},{'d'} });
	func(vec);
	std::size_t found_size = vec.size();
	assert(found_size == wcslen(expected_value));
	assert(std::equal(vec.cbegin(), vec.cend(), expected_value));
}
#define test_mutation(expected_value, op) \
_test_vec_mutation(expected_value, [=](test_vector_t& v){op;});
#define test_mutate_and(op, check)  {test_vector_t v; op; assert(check);}

template<class F>
void _test_vec_big_mutation(const wchar_t* expected_value, F func) {
	bigger_vector_t vec(L"abcd");
	func(vec);
	std::size_t found_size = vec.size();
	assert(found_size == wcslen(expected_value));
	assert(std::equal(vec.cbegin(), vec.cend(), expected_value));
}
#define test_big_mutation(expected_value, op)  _test_vec_big_mutation(expected_value, [=](bigger_vector_t& v){op;});

void test_vec_def_ctor() {
	test_vector_t vec{};
	assert(vec.size() == 0);
}

inline std::basic_istringstream<wchar_t> istream(const wchar_t* vec) {
	return std::basic_istringstream<wchar_t>(vec);
}

void test_small_vectors() {
	using ostream_t = std::basic_ostringstream<wchar_t>;
	std::vector<testing<0>> std_vector{ {'f'}, {'g'}, {'h'}, {'i'}, {'j'}, {'k'} };
	test_vector_t small_lvalue { {'l'}, {'m'}, {'n'}, {'o'}, {'p'}, {'q'} };
	bigger_vector_t large_lvalue { {'r'}, {'s'}, {'t'}, {'u'}, {'v' }, { 'w' }};
	std::initializer_list<testing<0>> init_list { {'x'}, {'y'}, {'z'}, {','}, {'.'}, {'/'} };
	std::hash<test_vector_t> hasher;

	test_vec_def_ctor();
	//test_vec_ctor(L"aaaa", 4, mpd::default_not_value_construct{});
	test_vec_ctor(L"12345", std::initializer_list<testing<0>>{ {'1'}, {'2'}, {'3'}, {'4'}, {'5' }, { '6' }, { '7' } });
	test_vec_ctor(L"lmnop", small_lvalue);
	test_vec_ctor(L"lmnop", std::move(small_lvalue));
	test_vec_ctor(L"xyz,.", init_list);
	test_vec_ctor(L"fghij", std_vector);
	test_vec_ctor(L"123", bigger_vector_t{ {'1'},{'2'},{'3' }});
	test_vec_ctor(L"rstuv", large_lvalue);
	test_vec_ctor(L"ABCDE", c_in_it{ 'A' }, c_in_it{ 'Z' });
	test_mutation(L"rstuv", v = large_lvalue);
	test_mutation(L"lmnop", v = small_lvalue);
	test_mutation(L"lmnop", v = std::move(small_lvalue));
	test_mutation(L"123", (v = { {'1'}, {'2'}, {'3'} }));
	test_mutation(L"xyz,.", v = init_list);
	test_mutation(L"fghij", v = std_vector);
	test_mutation(L"aaaa", v.assign(4, L'a'));
	test_mutation(L"aaaaa", v.assign(7, L'a'));
	test_mutation(L"12345", v.assign({ {'1'},{'2'},{'3'},{'4'},{'5'},{'6'},{'7'} }));
	test_mutation(L"xyz,.", v.assign(init_list));
	test_mutation(L"ABCDE", v.assign(c_in_it{ 'A' }, c_in_it{ 'Z' }));
	assert(small_lvalue.get_allocator() == std::allocator<char>{});
	assert(small_lvalue.at(2) == 'n');
	assert(small_lvalue[2] == 'n');
	assert(*small_lvalue.begin() == 'l');
	assert(*small_lvalue.cbegin() == 'l');
	auto it1 = small_lvalue.end();
	assert(*--it1 == 'p'); 
	auto it2 = small_lvalue.cend();
	assert(*--it2 == 'p');
	assert(*small_lvalue.rbegin() == 'p');
	assert(*small_lvalue.crbegin() == 'p');
	assert(*--small_lvalue.rend() == 'l');
	assert(*--small_lvalue.crend() == 'l');
	assert(test_vector_t({ 'a' }).empty() == false);
	assert(test_vector_t().empty() == true);
	assert(test_vector_t().size() == 0);
	assert(small_lvalue.size() == 5);
	assert(test_vector_t().capacity() == 5);
	test_mutate_and(v.reserve(4), v.capacity() == 5);
	test_mutate_and(v.shrink_to_fit(), v.capacity() == 5);
	test_mutate_and(v.assign({{'t'}, {'e'}, {'s'}, {'t'}}).clear(), v.size() == 0);
	test_mutation(L"azzbc", v.insert(v.begin()+1, 2, {'z'}));
	test_mutation(L"azzbc", v.insert(v.begin() + 1, {{ 'z' }, { 'z' }}));
	test_mutation(L"abzcd", v.insert(v.begin() + 2, { 'z' }));
	test_mutation(L"abzzc", v.insert(v.begin() + 2, 2, { 'z' }));
	test_mutation(L"abfgh", v.insert(v.begin() + 2, std_vector.begin(), std_vector.end()));
	test_mutation(L"abABC", v.insert(v.begin() + 2, c_in_it{ 'A' }, c_in_it{ 'Z' }));
	test_mutation(L"axyz,", v.insert(v.begin() + 1, init_list));
	test_mutation(L"ad", v.erase(v.begin() + 1, 2));
	test_mutation(L"bcd", v.erase(v.begin()));
	test_mutation(L"cd", v.erase(v.begin(), v.begin() + 2));
	test_mutation(L"abcdz", v.push_back('z'));
	test_mutation(L"abc", v.pop_back());
	test_mutation(L"ab", v.resize(2));
	test_mutation(L"abcd1", v.resize(6, '1'));
	//test_mutation(L"abcd1", v.resize(6, mpd::default_not_value_construct{}));
	test_mutation(L"lmnop", { auto o{small_lvalue}; v.swap(o); });
	assert((small_lvalue == large_lvalue) == false);
	assert((small_lvalue != large_lvalue) == true);
	assert((small_lvalue < large_lvalue) == true);
	assert((small_lvalue <= large_lvalue) == true);
	assert((small_lvalue > large_lvalue) == false);
	assert((small_lvalue >= large_lvalue) == false);
	assert((small_lvalue == std_vector) == false);
	assert((small_lvalue != std_vector) == true);
	assert((small_lvalue < std_vector) == false);
	assert((small_lvalue <= std_vector) == false);
	assert((small_lvalue > std_vector) == true);
	assert((small_lvalue >= std_vector) == true);
	test_mutation(L"abd", erase(v, L'c'));
	auto pred = [](const testing<0>& v) {return v == L'c'; };
	test_mutation(L"abd", erase_if(v, pred));
	assert(hasher({}) == 0x9e3779b9ull);
	assert(hasher({ {'a'},{'b'} }) == 4133876243914441702ull);
	assert(hasher({ {'b'},{'a'} }) == 4389997032588314883ull);
}