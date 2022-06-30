// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/read_stream.hh>
#include <cstring>

namespace cov::io {
	bool direct_read_stream::skip(size_t length) { return in_.skip(length); }

	size_t direct_read_stream::read(void* ptr, size_t length) {
		return in_ ? in_.load(ptr, length) : 0;
	}

	bool bytes_read_stream::skip(size_t length) {
		auto const chunk = std::min(length, in_.size());
		if (chunk != length) return false;

		in_ = in_.subview(chunk);
		return true;
	}

	size_t bytes_read_stream::read(void* ptr, size_t length) {
		auto const chunk = std::min(length, in_.size());
		std::memcpy(ptr, in_.data(), chunk);
		in_ = in_.subview(chunk);
		return chunk;
	}
}  // namespace cov::io
