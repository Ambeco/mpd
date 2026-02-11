
#include <cassert>
#include <fstream>
#include <iostream>
#include "inputoutput/async_ifilebuf.h"
#include "inputoutput/async_ofilebuf.h"

void assert_file_len(const char* filename, unsigned long expected_len) {
	std::ifstream outputFile(filename, std::ios_base::in | std::ios_base::binary);
	outputFile.seekg(0, std::ios::end);
	unsigned actual = outputFile.tellg();
	assert(actual == expected_len);
}
void assert_file_contents(const char* filename, unsigned long index, unsigned long length, const char* expected) {
	std::ifstream outputFile(filename, std::ios_base::in | std::ios_base::binary);
	outputFile.seekg(index, std::ios::beg);
	std::string buffer(length, 'B');
	outputFile.read(&buffer[0], buffer.size());
	assert(buffer.compare(0, length, expected) == 0);
}

void assert_file_contents(const char* filename, unsigned long index, unsigned long length, char character) {
	assert_file_contents(filename, index, length, std::string(length, character).c_str());
}

void test_async_iofilebuf() {
	{ // Assemble: create an inputfile to test with
		std::ofstream inputFile("inputfile.txt", std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
		for (unsigned i = 0; i < 5000; i++) {
			inputFile << std::setw(4) << i;
			inputFile.put('\n'); // force single newline characters, so files are deterministic
		}
	}
	assert_file_len("inputfile.txt", 25000);

	{ // Can read in, modify, and write out a file.
		mpd::async_ifilebuf ibuf("inputfile.txt", std::ios_base::in | std::ios_base::binary);
		std::istream inputFile(&ibuf);
		mpd::async_ofilebuf obuf("outputFile1.txt", std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
		std::ostream outputFile(&obuf);
		unsigned i = 0;
		while (inputFile >> i) {
			outputFile << std::setw(4) << i;
			outputFile.put('\n'); // force single newline characters, so files are deterministic
		}
	}
	assert_file_len("outputFile1.txt", 25000);

	{ // Can seek in both input and output streams
		mpd::async_ifilebuf ibuf("inputfile.txt", std::ios_base::in | std::ios_base::binary);
		std::istream inputFile(&ibuf);
		mpd::async_ofilebuf obuf("outputFile2.txt", std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
		std::ostream outputFile(&obuf);
		 // write 1000 'A' to the output file
		char buffer[1000] = {};
		std::fill(buffer, buffer + 1000, 'A');
		outputFile.write(buffer, 1000);
		// copy bytes 200-250 of input file into location 50 of output file (nums 40-50)
		inputFile.seekg(200); // ctor initiated read at location 0, so this tests ignoring that read
		inputFile.read(buffer, 50);
		assert(buffer[0] != 'A');
		outputFile.seekp(50); // the write above should have left some writes queued, so this tests that its correctly written before seek.
		outputFile.write(buffer, 50);
	}
	assert_file_contents("outputFile2.txt", 0, 50, 'A');
	assert_file_contents("outputFile2.txt", 50, 50, "  40\n  41\n  42\n  43\n  44\n  45\n  46\n  47\n  48\n  49\n");
	assert_file_contents("outputFile2.txt", 100, 50, 'A');
}