
#include <cassert>
#include <fstream>
#include <iostream>
#include "inputoutput/async_ifilebuf.h"
#include "inputoutput/async_ofilebuf.h"

void assert_file_len(const char* filename, unsigned long expected_len) {
	std::ifstream outputFile(filename);
	outputFile.seekg(0, std::ios::end);
	unsigned actual = outputFile.tellg();
	assert(actual == expected_len);
}
void assert_file_contents(const char* filename, unsigned long index, unsigned long length, const char* string) {
	std::ifstream outputFile(filename);
	outputFile.seekg(index, std::ios::beg);
	std::string buffer(length, '\0');
	outputFile.read(&buffer[0], buffer.size());
	assert(buffer == string);
}

void assert_file_contents(const char* filename, unsigned long index, unsigned long length, char character) {
	assert_file_contents(filename, index, length, std::string(length, character).c_str());
}

void test_async_iofilebuf() {
	{ // Assemble: create an inputfile to test with
		std::ofstream inputFile("inputfile.txt");
		for (unsigned i = 0; i < 100000; i++) {
			inputFile << i << '\n';
		}
	}
	assert_file_len("inputfile.txt", 688890);

	{ // Can read in, modify, and write out a file.
		mpd::async_ifilebuf ibuf("inputfile.txt");
		std::istream inputFile(&ibuf);
		mpd::async_ofilebuf obuf("outputFile.txt");
		std::ostream outputFile(&obuf);
		unsigned i = 0;
		while (inputFile >> i) {
			outputFile << (i * 2) << '\n';
		}
	}
	assert_file_len("outputFile.txt", 744445);

	/* TODO:
	{ // Can seek in both input and output streams
		mpd::async_ifilebuf ibuf("inputfile.txt");
		std::istream inputFile(&ibuf);
		mpd::async_ofilebuf obuf("outputFile.txt");
		std::ostream outputFile(&obuf);
		 // write 1000 spaces to the output file
		char buffer[100] = {};
		std::fill(buffer, buffer + 100, ' ');
		for (int i = 0; i < 10; i++)
			outputFile.write(buffer, 100);
		// seek into in both files, and copy 100 bytes
		inputFile.seekg(200);
		outputFile.seekp(100);
		inputFile.read(buffer, 100);
		assert(buffer[0] != ' ');
		outputFile.write(buffer, 100);
	}
	assert_file_contents("outputFile.txt", 0, 100, ' ');
	assert_file_contents("outputFile.txt", 200, 50, ' ');
	assert_file_contents("outputFile.txt", 200, 100, '1');
	*/
}