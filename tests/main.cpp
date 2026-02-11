#include <iostream>

void test_istream_lit();
void test_small_strings();
void test_erasable();
void test_pimpl();
void test_pimpl2();
void test_small_vectors();
void test_bitfields();
void test_atomic_spin();
void test_async_iofilebuf();

int main() {
	std::cout << "Starting tests..." << std::endl;
	test_istream_lit();
	test_small_strings();
	test_erasable();
	test_pimpl();
	test_pimpl2();
	test_small_vectors();
	test_bitfields();
	test_atomic_spin();
	test_async_iofilebuf();
	std::cout << "Success\n";
	return 0;
}