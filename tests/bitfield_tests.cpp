
#include <cassert>
#include "containers/bitfield.hpp"

class TestBitfield {
	using A_type = mpd::bitfield_member<unsigned long long, bool, 1, 0>;
	using B_type = mpd::bitfield_member<unsigned long long, short, 10, 1>;
	using C_type = mpd::bitfield_member<unsigned long long, unsigned short, 9, 11>;
	unsigned long long bits;
public:
	TestBitfield(unsigned long long b = 0) : bits(b) {}
	A_type A() { return A_type(bits); }
	A_type::as_const A() const { return A_type::as_const(bits); }
	B_type B() { return B_type(bits); }
	B_type::as_const B() const { return B_type::as_const(bits); }
	C_type C() { return C_type(bits); }
	C_type::as_const C() const { return C_type::as_const(bits); }

	bool operator==(const unsigned long long other) const { return bits == other; }
};

void test_bitfields() {
	TestBitfield test;
	assert(test == 0);
	assert(test.A() == false);
	assert(test.B() == 0);
	assert(test.C() == 0);

	test.C() = 0x101; // 9bits: 1'0000'0001
	assert(test == 0x80800); // 20bits: 1000'0000'1000'0000'0000
	assert(test.C() == 0x101);

	test.B() = 0x101; // 10bits: 01'0000'0001
	assert(test == 0x80A02); // 20bits: 1000'0000'1010'0000'0010
	assert(test.B() == 0x101);

	test.B() = -1; // 10bits: 11'1111'1111
	assert(test == 0x80FFE); // 20bits: 1000'0000'1111'1111'1110
	assert(test.B() == -1);

	test.A() = true; // 1bits: 1
	assert(test == 0x80FFF); // 20bits: 1000'0000'1111'1111'1111
	assert(test.A() == true);

	// TODO: test all the other members
}

