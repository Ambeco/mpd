#include <cmath>
#include <limits>
#include "utilities/macros.hpp"

namespace mpd {
	namespace impl {
		template<class BaseT, class FieldT>
		struct bitfield_getter {
			static_assert(std::is_integral<BaseT>::value, "non-integer types require a custom bitfield getter");
			FieldT operator()(BaseT value) const {
				return static_cast<FieldT>(value);
			}
		};
		template<class BaseT, class FieldT>
		struct bitfield_setter {
			static_assert(std::is_integral<BaseT>::value, "non-integer types require a custom bitfield getter");
			BaseT operator()(FieldT value) const {
				return static_cast<BaseT>(value);
			}
		};
	}
	template<class BaseT,
		class FieldT,
		unsigned bitcount,
		unsigned offset,
		class Getter = mpd::impl::bitfield_getter<BaseT, FieldT>,
		class Setter = mpd::impl::bitfield_setter<BaseT, FieldT>>
		class bitfield_member;
}

namespace std {
	// TODO: how to handle floats?
	template<class BaseT, class FieldT, unsigned bitcount, unsigned offset, class Getter, class Setter>
	class numeric_limits<mpd::bitfield_member<BaseT, FieldT, bitcount, offset, Getter, Setter>>
		: numeric_limits<FieldT> {
		static constexpr double log10_2 = 0.32221929473; // we only need ~3 digits for calculating digits10
		static const BaseT lomask = (static_cast<BaseT>(1u) << bitcount) - 1u;
	public:
		static const bool is_iec599 = false;
		static const unsigned digits = bitcount;
		static const unsigned digits10 = static_cast<unsigned>(digits * log10_2);
		static constexpr FieldT min() {
			if (numeric_limits<FieldT>::is_signed) {
				return Getter{}(static_cast<BaseT>(1u) << (bitcount - 1u));
			} else {
				return Getter{}(static_cast<BaseT>(0u));
			}
		}
		static constexpr FieldT max() {
			if (numeric_limits<FieldT>::is_signed) {
				return Getter{}(lomask >> 1u);
			} else {
				return Getter{}(lomask);
			}
		}
		static constexpr FieldT lowest() { return min(); }
	};
}

namespace mpd {
// Make a bitfield class that has a private BaseT bitfield.
// Then, for each bitfield member, make a method that returns a bitfield_member
// for that.
//
// Ex:
// class MyBitfield {
//    using A_type = bitfield_member<unsigned long long, bool, 0, 1>;
//    using A_const = bitfield_member<unsigned long long, const bool, 0, 1>;
//    using B_type = bitfield_member<unsigned long long, unsigned short, 1, 16>
//    unsigned long long bits;
// public:
//    A_type A() {return {bits};}
//    A_const A() const {return {bits};}
//    B_type B() {return {bits};}
//    B_const B() const {return {bits};}
// };
//
	template<class BaseT, class FieldT, unsigned bitcount, unsigned offset, class Getter, class Setter>
	class bitfield_member : Getter, Setter {
		static_assert(std::numeric_limits<BaseT>::is_exact
			&& std::numeric_limits<BaseT>::is_integer
			&& !std::numeric_limits<BaseT>::is_signed,
			"BaseT must be an unsigned integer type");
		static_assert(bitcount > 0, "bitfield member must have a positive number of bits");
		static_assert(bitcount <= sizeof(BaseT) * CHAR_BIT, "bitfield member cannot have more bits than BaseT");
		static_assert(bitcount + offset <= sizeof(BaseT) * CHAR_BIT, "bitfield member extends past the size of BaseT");

		static constexpr BaseT lomask = (static_cast<BaseT>(1ull) << bitcount) - 1ull;
		static constexpr FieldT fieldMin = std::numeric_limits<FieldT>::min();
		static constexpr FieldT fieldMax = std::numeric_limits<FieldT>::max();
		BaseT* bits;
	public:
		using as_const = bitfield_member<const BaseT, const FieldT, bitcount, offset, Getter, Setter>;

		constexpr explicit bitfield_member(BaseT& bits_) :bits(&bits_) {}
		constexpr operator FieldT() const { return get(); }
		constexpr FieldT get() const {
			// Despite appearances, this whole method is branchless
			BaseT raw_bits = (*bits >> offset) & lomask;
			BaseT sign_extended_bits;
			if (std::numeric_limits<FieldT>::is_signed) {
				// negative values need the sign-extension bits set
				sign_extended_bits = ~((raw_bits >> (bitcount - 1)) - 1u);
				sign_extended_bits = (sign_extended_bits & ~lomask) | raw_bits;
			} else {
				sign_extended_bits = raw_bits;
			}
			return Getter::operator()(sign_extended_bits);
		}
		constexpr void set(FieldT value) {
			// Despite appearances, this whole method is branchless

			// ensure that input value is in the range of values that fit
			// note: negative values may have sign-extension bits more than bitcount,
			// but these are not significant bits.
			assume(value >= fieldMin && value <= fieldMax);
			BaseT resetBits = *bits & ~(lomask << offset);
			BaseT newBits = Setter::operator()(value);
			// ensure that Setter result's significant bits fit in the target size
			// here again, negative values may have sign-extension bits outside the valid range
			// but as we already validated the range, we can drop those sign-extension bits.
			if (std::numeric_limits<FieldT>::is_signed) {
				assume((newBits | lomask) == lomask || ~(newBits | lomask) == 0u);
				newBits &= lomask;
			} else {
				assume((newBits >> offset >> bitcount) == 0u);
			}
			*bits = resetBits | (newBits << offset);
		}
		constexpr bitfield_member& operator=(FieldT value) { set(value); return *this; }
		constexpr bitfield_member& operator++() { set(get()++); return *this; }
		constexpr FieldT operator++(int) { FieldT t = get(); set(get()++); return t; }
		constexpr bitfield_member& operator--() { set(get()--); return *this; }
		constexpr FieldT operator--(int) { FieldT t = get(); set(get()--); return t; }
		constexpr FieldT operator+() const { return +get(); }
		constexpr FieldT operator-() const { return -get(); }
		constexpr FieldT operator!() const { return !get(); }
		constexpr FieldT operator~() const { return ~get(); }
		constexpr FieldT operator*(FieldT value) const { return get() * value; }
		constexpr bitfield_member& operator*=(FieldT value) { set(get() * value); return *this; }
		constexpr FieldT operator/(FieldT value) const { return get() / value; }
		constexpr bitfield_member& operator/=(FieldT value) { set(get() / value); return *this; }
		constexpr FieldT operator%(FieldT value) const { return get() % value; }
		constexpr bitfield_member& operator%=(FieldT value) { set(get() % value); return *this; }
		constexpr FieldT operator+(FieldT value) const { return get() + value; }
		constexpr bitfield_member& operator+=(FieldT value) { set(get() + value); return *this; }
		constexpr FieldT operator-(FieldT value) const { return get() - value; }
		constexpr bitfield_member& operator-=(FieldT value) { set(get() - value); return *this; }
		constexpr FieldT operator<<(FieldT value) const { return get() << value; }
		constexpr bitfield_member& operator<<=(FieldT value) { set(get() <<= value); return *this; }
		constexpr FieldT operator>>(FieldT value) const { return get() >> value; }
		constexpr bitfield_member& operator>>=(FieldT value) { set(get() >>= value); return *this; }
		//constexpr bool operator<(FieldT value) const { return get() < value; }
		//constexpr bool operator<=(FieldT value) const { return get() <= value; }
		//constexpr bool operator>(FieldT value) const { return get() > value; }
		//constexpr bool operator>=(FieldT value) const { return get() >= value; }
		//constexpr bool operator==(FieldT value) const { return get() == value; }
		//constexpr bool operator!=(FieldT value) const { return get() != value; }
		constexpr FieldT operator&(FieldT value) const { return get() & value; }
		constexpr bitfield_member& operator&=(FieldT value) { set(get() & value); return *this; }
		constexpr FieldT operator^(FieldT value) const { return get() ^ value; }
		constexpr bitfield_member& operator^=(FieldT value) { set(get() ^ value); return *this; }
		constexpr FieldT operator|(FieldT value) const { return get() | value; }
		constexpr bitfield_member& operator|=(FieldT value) { set(get() | value); return *this; }
	};
}