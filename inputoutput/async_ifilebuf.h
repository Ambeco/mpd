#pragma once
//double buffer input filebuf by Tavis Bohne
//as a heavy rewrite of  https://stackoverflow.com/a/21127776/845092
//which was written by Dietmar Kühl Jan 15 '14

#include <fstream>
#include <future>
#include <streambuf>
#include <vector>

namespace mpd {
// std::streambuf that pre-reads up to 4kb in a background thread for istream, so that the calling thread
// is unlikely to stall. When reads are interlaced with cpu usage, this can greatly improve performance.
// When reads are not interlaced with cpu usage, this will slightly hinder performance.
// ex:
// async_ifilebuf stream_buf(in_path.c_str(), std::ios_base::binary);
// std::istream stream(&stream_buf);
// MyClass myData;
// while (stream >> myData) {
//     doStuff(myData);
// }
	struct async_ifilebuf : virtual std::streambuf {
		const std::size_t buffer_dump_size = 4096;

		std::ifstream in; // TODO :replace with std::filebuf
		std::vector<char> filling_buffer;
		std::vector<char> dumping_buffer;
		std::future<void> fill_future;

		void worker() {
			filling_buffer.resize(buffer_dump_size);
			in.read(filling_buffer.data(), filling_buffer.size());
			filling_buffer.resize(in.gcount());
		}
	public:
		async_ifilebuf(const char* name, std::ios_base::openmode mode = std::ios_base::in) {
			fill_future = std::async(std::launch::async, [this, name, mode]() {in.open(name, mode); worker(); });
		}
		async_ifilebuf(const wchar_t* name, std::ios_base::openmode mode = std::ios_base::in) {
			fill_future = std::async(std::launch::async, [this, name, mode]() {in.open(name, mode); worker(); });
		}
		async_ifilebuf(async_ifilebuf&& rhs) noexcept {
			rhs.sync();
			operator=(std::move(rhs));
		}
		~async_ifilebuf() noexcept {
			close();
		}
		async_ifilebuf& operator=(async_ifilebuf&& rhs) noexcept {
			close();
			in = std::move(rhs.in);
			filling_buffer = std::move(rhs.filling_buffer);
			dumping_buffer = std::move(rhs.dumping_buffer);
			fill_future = std::move(rhs.fill_future);
			return *this;
		}
		void close() {
			if (fill_future.valid()) fill_future.get();
			in.close();
		}
		int underflow() {
			char* ptr = gptr();
			if (ptr != nullptr && ptr != egptr()) {
				unsigned v = (unsigned) *ptr;
				return v;
			}
			if (fill_future.valid()) fill_future.get();
			dumping_buffer.swap(filling_buffer);
			if (filling_buffer.empty() && in.eof()) return std::char_traits<char>::eof();
			if (!dumping_buffer.empty()) 
				fill_future = std::async(std::launch::async, [this]() { worker(); });
			setg(dumping_buffer.data(), dumping_buffer.data(), dumping_buffer.data() + dumping_buffer.size());
			return (unsigned) *dumping_buffer.data();
		}
	};
}