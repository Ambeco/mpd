#pragma once
//double buffer output filebuf by Tavis Bohne
//as a heavy rewrite of  https://stackoverflow.com/a/21127776/845092
//which was written by Dietmar Kühl Jan 15 '14

#include <fstream>
#include <future>
#include <iomanip>
#include <streambuf>
#include <vector>

namespace mpd {
// std::streambuf that pre-reads up to 4kb in a background thread for istream, so that the calling thread
// is unlikely to stall. When reads are interlaced with cpu usage, this can greatly improve performance.
// When reads are not interlaced with cpu usage, this will slightly hinder performance.
// ex:
// MyClass myData;
// async_ofilebuf stream_buf(in_path.c_str(), std::ios_base::binary);
// std::ostream stream(&stream_buf);
// for (foo : myData) {
//   stream << myData.calculateStuff() << '\n';
// }
	struct async_ofilebuf : virtual std::streambuf {
		const std::size_t buffer_dump_size = 4096;

		std::ofstream out; // TODO :replace with std::filebuf
		std::vector<char> filling_buffer;
		std::vector<char> dumping_buffer;
		std::future<void> dump_future;

		void worker() {
			out.write(dumping_buffer.data(), dumping_buffer.size());
			dumping_buffer.clear();
		}

		void dump() {
			std::size_t filling_count = std::size_t(pptr() - pbase());
			if (filling_count == 0 && !dump_future.valid()) return;
			filling_buffer.resize(filling_count);
			if (dump_future.valid()) dump_future.get();
			filling_buffer.swap(dumping_buffer);
			if (!dumping_buffer.empty()) 
				dump_future = std::async(std::launch::async, [this]() {worker(); });
			filling_buffer.resize(buffer_dump_size);
			setp(filling_buffer.data(), filling_buffer.data() + filling_buffer.size() - 1);
		}
	public:
		async_ofilebuf(const char* name, std::ios_base::openmode mode = std::ios_base::out)
			: filling_buffer(buffer_dump_size) {
			setp(filling_buffer.data(), filling_buffer.data() + filling_buffer.size() - 1);
			dump_future = std::async(std::launch::async, [this, name, mode]() {out.open(name, mode); });
		}
		async_ofilebuf(const wchar_t* name, std::ios_base::openmode mode = std::ios_base::out)
			: filling_buffer(buffer_dump_size) {
			setp(filling_buffer.data(), filling_buffer.data() + filling_buffer.size() - 1);
			dump_future = std::async(std::launch::async, [this, name, mode]() {out.open(name, mode); });
		}
		async_ofilebuf(async_ofilebuf&& rhs) noexcept {
			rhs.sync();
			operator=(std::move(rhs));
		}
		~async_ofilebuf() noexcept {
			close();
		}
		async_ofilebuf& operator=(async_ofilebuf&& rhs) noexcept {
			close();
			rhs.sync();
			out = std::move(rhs.out);
			filling_buffer = std::move(rhs.filling_buffer);
			dumping_buffer = std::move(rhs.dumping_buffer);
			dump_future = std::move(rhs.dump_future);
			return *this;
		}
		void close() {
			sync();
			out.close();
		}
		int overflow(int c) override {
			if (c != std::char_traits<char>::eof()) {
				dump();
				*pptr() = std::char_traits<char>::to_char_type(c);
				pbump(1);
				return std::char_traits<char>::not_eof(c);
			} else {
				sync();
				return c;
			}
		}
		int sync() override {
			dump();
			if (dump_future.valid()) dump_future.get();
			out.flush();
			return 0;
		}
		pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode = std::ios_base::out) override {
			dump(); // initiate dump of any pending writes
			dump_future = std::async(std::launch::async, [this, off, dir, old_dump_future=std::move(dump_future)]() mutable {
				if (old_dump_future.valid()) old_dump_future.get(); // once that's done, THEN seek, but we don't have to block calling thread for that
				out.seekp(off, dir); 
			});
			setp(dumping_buffer.data(), dumping_buffer.data(), dumping_buffer.data());
			return out.tellp();
		}
		pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::out) override {
			return seekoff(pos, std::ios_base::beg, which);
		}
	};
}