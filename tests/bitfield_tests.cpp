
#include <cassert>
#include "containers/bitfield.hpp"

class TestTemplateBitfield {
	using A_type = mpd::bitfield_member<unsigned long long, bool, 1, 0>;
	using B_type = mpd::bitfield_member<unsigned long long, short, 10, 1>;
	using C_type = mpd::bitfield_member<unsigned long long, unsigned short, 9, 11>;
	unsigned long long bits;
public:
	TestTemplateBitfield(unsigned long long b = 0) : bits(b) {}
	A_type A() { return A_type(bits); }
	A_type::as_const A() const { return A_type::as_const(bits); }
	B_type B() { return B_type(bits); }
	B_type::as_const B() const { return B_type::as_const(bits); }
	C_type C() { return C_type(bits); }
	C_type::as_const C() const { return C_type::as_const(bits); }

	bool operator==(const unsigned long long other) const { return bits == other; }
};

void test_template_bitfields() {
	TestTemplateBitfield test;
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


class Board {
	unsigned long long m_array = 0ull;
public:
	enum Color { White, Black };
	enum Kind { None, King, Queen, Rook, Bishop, Knight, Pawn };
	template<class underlying>
	class PieceRef {
		using ColorRef = mpd::bitfield_member<underlying, Color, 1>;
		using KindRef = mpd::bitfield_member<underlying, Kind, 3>;
		underlying* const bits;
		const unsigned offset;
	public:
		MPD_INLINE() PieceRef(underlying* bits_, unsigned offset_) :bits(bits_), offset(offset_) {}

		MPD_INLINE(ColorRef) color() const { return ColorRef{*bits, offset}; }
		MPD_INLINE(KindRef) kind() const { return KindRef{*bits, offset + 1u}; }
		MPD_INLINE(void) set(Color color_, Kind kind_) { color() = color_; kind() = kind_; }
		MPD_INLINE(void) clear() { kind() = None; }
	};

	MPD_INLINE(PieceRef<unsigned long long>) operator[](unsigned row) { return {&m_array, row* 4u}; }
	MPD_INLINE(PieceRef<const unsigned long long>) operator[](unsigned row) const { return {&m_array, row* 4u}; }
};

void test_dynamic_bitfields() {
	Board test;
	assert(test[0].color() == Board::White);
	assert(test[0].kind() == Board::None);
	assert(test[7].color() == Board::White);
	assert(test[7].kind() == Board::None);

	test[0].kind() = Board::Pawn;
	assert(test[0].color() == Board::White);
	assert(test[0].kind() == Board::Pawn);
	assert(test[7].color() == Board::White);
	assert(test[7].kind() == Board::None);

	test[7].color() = Board::Black;
	assert(test[0].color() == Board::White);
	assert(test[0].kind() == Board::Pawn);
	assert(test[7].color() == Board::Black);
	assert(test[7].kind() == Board::None);

	// TODO: test all the other members
}

void test_bitfields() {
	test_template_bitfields();
	test_dynamic_bitfields();
}

