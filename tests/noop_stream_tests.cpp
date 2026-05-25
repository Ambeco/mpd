#include "inputoutput/noop_stream.hpp"

void test_noop_stream() {
	mpd::noop_stream << "This should not be printed" << std::endl;
	mpd::noop_stream << 12345 << std::endl;
	mpd::noop_stream << 3.14159 << std::endl;
	mpd::noop_stream << (void*)0xDEADBEEF << std::endl;
}