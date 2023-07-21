// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "test_stream.hh"

namespace cov::testing {
	bool test_stream::opened() const noexcept { return true; }
	size_t test_stream::write(git::bytes block) {
		if (free_size != infinite) {
			auto chunk = block.size();
			if (chunk > free_size) chunk = free_size;
			free_size -= chunk;
			block = block.subview(0, chunk);
		}
		auto const size = data.size();
		data.insert(data.end(), block.begin(), block.end());
		return data.size() - size;
	}

	std::string_view test_stream::view() const noexcept {
		return {reinterpret_cast<char const*>(data.data()), data.size()};
	}
}  // namespace cov::testing
