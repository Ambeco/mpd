
void test_istream_lit();
void test_small_strings();
void test_erasable();
void test_pimpl();
void test_pimpl2();
void test_small_vectors();
void test_bitfields();

int main() {
	test_istream_lit();
	test_small_strings();
	test_erasable();
	test_pimpl();
	test_pimpl2();
	test_small_vectors();
	test_bitfields();
	return 0;
}