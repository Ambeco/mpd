#pragma once
//double buffer output filebuf by Tavis Bohne
//as a heavy rewrite of  https://stackoverflow.com/a/21127776/845092
//which was written by Dietmar Kühl Jan 15 '14

#include <fstream>
#include <future>
#include <iomanip>
#include <streambuf>
#include <utility>
#include <vector>

namespace mpd {
// std::streambuf that buffers up to 4kb and writes it out in a background thread for ostream, so that the
// calling thread is unlikely to stall. When writes are interlaced with cpu usage, this can greatly improve
// performance. When writes are not interlaced with cpu usage, this will slightly hinder performance.
// buf_type is the streambuf delegated to for the actual writes/seeks. Either move in an already-open one to
// delegate to it, or pass buf_type::open()'s own arguments (e.g. a filename) to have it opened for you --
// either way, buf_type must have open()/close() (e.g. std::filebuf; see async_ofilebuf below). close()
// closes it too.
// ex:
// async_obuf<std::filebuf> stream_buf(std::move(myFilebuf));
// std::ostream stream(&stream_buf);
// for (foo : myData) {
//   stream << myData.calculateStuff() << '\n';
// }
	template <class buf_type>
	struct async_obuf : virtual std::streambuf {
		const std::size_t buffer_dump_size = 4096;

		buf_type out;
		std::vector<char> filling_buffer;
		std::vector<char> dumping_buffer;
		std::future<void> dump_future;

		void worker() {
			out.sputn(dumping_buffer.data(), std::streamsize(dumping_buffer.size()));
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
		void start() {
			filling_buffer.resize(buffer_dump_size);
			setp(filling_buffer.data(), filling_buffer.data() + filling_buffer.size() - 1);
		}
	public:
		async_obuf() {
			start();
		}
		explicit async_obuf(buf_type&& already_open) : out(std::move(already_open)) {
			start();
		}
		template <class First, class... Rest>
		explicit async_obuf(First&& first, Rest&&... rest) {
			start();
			dump_future = std::async(std::launch::async, [this, first, rest...]() { out.open(first, rest...); });
		}
		async_obuf(async_obuf&& rhs) noexcept {
			rhs.sync();
			out = std::move(rhs.out);
			filling_buffer = std::move(rhs.filling_buffer);
			dumping_buffer = std::move(rhs.dumping_buffer);
			dump_future = std::move(rhs.dump_future);
			setp(filling_buffer.data(), filling_buffer.data() + filling_buffer.size() - 1);
		}
		~async_obuf() noexcept {
			close();
		}
		async_obuf& operator=(async_obuf&& rhs) noexcept {
			if (this == &rhs) return *this;
			close();
			rhs.sync();
			out = std::move(rhs.out);
			filling_buffer = std::move(rhs.filling_buffer);
			dumping_buffer = std::move(rhs.dumping_buffer);
			dump_future = std::move(rhs.dump_future);
			setp(filling_buffer.data(), filling_buffer.data() + filling_buffer.size() - 1);
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
			out.pubsync();
			return 0;
		}
		pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode = std::ios_base::out) override {
			dump(); // flushes any pending writes and resets the put area to a fresh filling_buffer
			if (dump_future.valid()) dump_future.get(); // must finish before we seek, since it's writing through `out` too
			return out.pubseekoff(off, dir, std::ios_base::out);
		}
		pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::out) override {
			return seekoff(pos, std::ios_base::beg, which);
		}
	};

	using async_ofilebuf = async_obuf<std::filebuf>;
}
