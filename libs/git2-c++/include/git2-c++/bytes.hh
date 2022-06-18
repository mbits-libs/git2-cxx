// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/buffer.h>
#include <cstddef>

namespace git {
	struct bytes {
		using size_type = size_t;
		bytes() = default;
		bytes(bytes const&) = default;
		bytes(bytes&&) = default;
		bytes& operator=(bytes const&) = default;
		bytes& operator=(bytes&&) = default;

		constexpr bytes(std::byte const* data, size_type length) noexcept
		    : data_{data}, length_{length} {}

		bytes(char const* data, size_type length) noexcept
		    : data_{reinterpret_cast<std::byte const*>(data)}
		    , length_{length} {}

		explicit bytes(git_buf const& buf) noexcept
		    : bytes(buf.ptr, static_cast<size_type>(buf.size)) {}

		constexpr std::byte const* data() const noexcept { return data_; }
		constexpr std::byte const* begin() const noexcept { return data_; }
		constexpr std::byte const* end() const noexcept {
			return data_ + length_;
		}
		constexpr size_type size() const noexcept { return length_; }

	private:
		std::byte const* data_{};
		size_type length_{};
	};
}  // namespace git
