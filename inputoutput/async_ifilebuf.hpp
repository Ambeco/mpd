#pragma once
//double buffer input filebuf by Tavis Bohne
//as a heavy rewrite of  https://stackoverflow.com/a/21127776/845092
//which was written by Dietmar K�hl Jan 15 '14

#include <fstream>
#include <future>
#include <streambuf>
#include <utility>
#include <vector>

namespace mpd {
// std::streambuf that pre-reads up to 4kb in a background thread for istream, so that the calling thread
// is unlikely to stall. When reads are interlaced with cpu usage, this can greatly improve performance.
// When reads are not interlaced with cpu usage, this will slightly hinder performance.
// buf_type is the streambuf delegated to for the actual reads/seeks. Either move in an already-open one to
// delegate to it, or pass buf_type::open()'s own arguments (e.g. a filename) to have it opened for you --
// either way, buf_type must have open()/close() (e.g. std::filebuf; see async_ifilebuf below). close()
// closes it too.
// ex:
// async_ibuf<std::filebuf> stream_buf(std::move(myFilebuf));
// std::istream stream(&stream_buf);
// MyClass myData;
// while (stream >> myData) {
//     doStuff(myData);
// }
	template <class buf_type>
	struct async_ibuf : virtual std::streambuf {
		const std::size_t buffer_dump_size = 4096;

		buf_type in;
		std::vector<char> filling_buffer;
		std::vector<char> dumping_buffer;
		std::future<void> fill_future;

		void worker() {
			filling_buffer.resize(buffer_dump_size);
			std::streamsize got = in.sgetn(filling_buffer.data(), std::streamsize(filling_buffer.size()));
			filling_buffer.resize(std::size_t(got));
		}
		void start() {
			fill_future = std::async(std::launch::async, [this]() { worker(); });
		}
	public:
		explicit async_ibuf(buf_type&& already_open) : in(std::move(already_open)) {
			start();
		}
		template <class First, class... Rest>
		explicit async_ibuf(First&& first, Rest&&... rest) {
			fill_future = std::async(std::launch::async, [this, first, rest...]() { in.open(first, rest...); worker(); });
		}
		async_ibuf(async_ibuf&& rhs) noexcept {
			operator=(std::move(rhs));
		}
		~async_ibuf() noexcept {
			close();
		}
		async_ibuf& operator=(async_ibuf&& rhs) noexcept {
			if (this == &rhs) return *this;
			close();
			if (rhs.fill_future.valid()) rhs.fill_future.get(); // join rhs's background worker before touching its members below
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
		int underflow() override {
			char* ptr = gptr();
			if (ptr != nullptr && ptr != egptr()) {
				unsigned v = (unsigned) *ptr;
				return v;
			}
			if (fill_future.valid()) fill_future.get();
			dumping_buffer.swap(filling_buffer);
			if (dumping_buffer.empty()) return std::char_traits<char>::eof();
			fill_future = std::async(std::launch::async, [this]() { worker(); });
			setg(dumping_buffer.data(), dumping_buffer.data(), dumping_buffer.data() + dumping_buffer.size());
			return (unsigned) *dumping_buffer.data();
		}
		pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode = std::ios_base::in) override {
			if (fill_future.valid()) fill_future.get();
			// Seek synchronously (so the returned position is correct, and `in` isn't touched by two threads
			// at once), then kick off the read-ahead for the new position in the background.
			pos_type result = in.pubseekoff(off, dir, std::ios_base::in);
			setg(dumping_buffer.data(), dumping_buffer.data(), dumping_buffer.data());
			fill_future = std::async(std::launch::async, [this]() { worker(); });
			return result;
		}
		pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in) override {
			return seekoff(pos, std::ios_base::beg, which);
		}
	};

	using async_ifilebuf = async_ibuf<std::filebuf>;
}
