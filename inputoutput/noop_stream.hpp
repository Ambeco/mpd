#pragma once
#include <ostream>

namespace mpd {
	// Useful for disabling output in release builds
	// 
	// #ifdef NDEBUG
	// #define dbgOut mpd::noop_stream
	// #else
	// #define dbgOut std::cout
	// #endif
	struct noop_stream_impl : public std::ostream {
		struct noop_streambuf : public std::streambuf {
			int overflow(int c) override { return traits_type::not_eof(c); }
		} streambuf;
		noop_stream_impl() noexcept : std::ostream(&streambuf) {}
		noop_stream_impl(std::basic_streambuf<char_type, traits_type>*) noexcept : std::ostream(&streambuf) {}
		noop_stream_impl(noop_stream_impl&& rhs) noexcept : std::ostream(&streambuf) {}
		noop_stream_impl& operator<<(bool) noexcept { return *this; }
		noop_stream_impl& operator<<(long) noexcept { return *this; }
		noop_stream_impl& operator<<(unsigned long) noexcept { return *this; }
		noop_stream_impl& operator<<(long long) noexcept { return *this; }
		noop_stream_impl& operator<<(unsigned long long) noexcept { return *this; }
		noop_stream_impl& operator<<(double) noexcept { return *this; }
		noop_stream_impl& operator<<(long double) noexcept { return *this; }
		noop_stream_impl& operator<<(const void*) noexcept { return *this; }
		noop_stream_impl& operator<<(const volatile void*) noexcept { return *this; }
		noop_stream_impl& operator<<(std::nullptr_t) noexcept { return *this; }
		noop_stream_impl& operator<<(short) noexcept { return *this; }
		noop_stream_impl& operator<<(int) noexcept { return *this; }
		noop_stream_impl& operator<<(unsigned short) noexcept { return *this; }
		noop_stream_impl& operator<<(unsigned int) noexcept { return *this; }
		noop_stream_impl& operator<<(float) noexcept { return *this; }
		noop_stream_impl& operator<<(std::basic_streambuf<char_type, traits_type>*) noexcept { return *this; }
		noop_stream_impl& operator<<(std::ios_base& (*)(std::ios_base&)) noexcept { return *this; }
		noop_stream_impl& operator<<(std::basic_ios<char_type, traits_type>& (*)(std::basic_ios<char_type, traits_type>&)) noexcept { return *this; }
		noop_stream_impl& operator<<(std::basic_ostream<char_type, traits_type>& (*) (std::basic_ostream<char_type, traits_type>&)) noexcept { return *this; }
	} noop_stream;
	template<class T>
	noop_stream_impl& operator<<(noop_stream_impl& o, const T&) noexcept { return o; }
}
