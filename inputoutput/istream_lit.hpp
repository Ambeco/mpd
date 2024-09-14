#pragma once
#include <array>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

/**
* Simple helper header that allows you to stream in string and character literals
**/

template<class e, class t, class array>
std::enable_if_t<
	std::is_same_v<const e, std::remove_extent_t<array>>
	, std::basic_istream<e, t>&>
	operator>>(std::basic_istream<e, t>& in, array& str_lit) {
	static const std::size_t N = std::extent_v<array>;
	static_assert(N > 0, "array must have a positive length");
	typename std::basic_istream<e, t>::sentry sentry(in, !std::isspace(str_lit[0]));
	for (std::size_t i = 0; i < N - 1; i++) {
		e char_lit = str_lit[i];
		if (in.peek() == char_lit)
			in.get();
		else {
			in.setstate(in.rdstate() | std::ios::badbit);
			break;
		}
	}
	return in;
}

template<class e, class t>
std::basic_istream<e, t>& operator>>(std::basic_istream<e, t>& in, const e& char_lit) {
	typename std::basic_istream<e, t>::sentry sentry(in, !std::isspace(char_lit));
	if (in.peek() == char_lit)
		in.get();
	else
		in.setstate(in.rdstate() | std::ios::badbit); //set the state
	return in;
}

/*
* The std istream operator>>(e*) overload is always selected over this
template<class e, class t, class array>
std::enable_if_t<
	!std::is_same_v<const e, std::remove_extent_t<array>>
	, std::basic_istream<e, t>&>
	operator>>(std::basic_istream<e, t>& in, array& mutable_array) {
	static const std::size_t N = std::extent_v<array>;
	static_assert(N > 0, "array must have a positive length");
	std::memset(mutable_array, 0, N * sizeof(e));
	typename std::basic_istream<e, t>::sentry sentry(in, false);
	for (std::size_t i = 0; i < N; i++) {
		e char_lit = str_lit[i];
		e peek = in.peek();
		if (peek == std::ios::eof() || std::isspace(peek))
			break;
		else if (i == N - 1) { //there's still non-space characters after end of buffer
			in.setstate(in.rdstate() | std::ios::badbit);
			break;
		} else 
			mutable_array[i] = peek;
	}
	return in;
}
*/
