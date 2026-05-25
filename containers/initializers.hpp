#pragma once
#include <cstddef>
#include <type_traits>

namespace mpd {
	struct reserved {
		std::size_t capacity;

		template<class Container, class enabled=decltype(std::declval<Container>().reserve(static_cast<std::size_t>(1)))>
		operator Container() const {
			Container c;
			c.reserve(capacity);
			return c;
		}
	};
}