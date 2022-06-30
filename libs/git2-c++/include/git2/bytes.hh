// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/buffer.h>
#include <concepts>
#include <cstddef>
#include <string_view>

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

		template <typename Byte>
		requires(!std::same_as<std::byte, Byte> && (sizeof(Byte) == 1))
		    bytes(Byte const* data, size_type length) noexcept
		    : data_{reinterpret_cast<std::byte const*>(data)}
		    , length_{length} {}

		explicit bytes(git_buf const& buf) noexcept
		    : bytes(buf.ptr, static_cast<size_type>(buf.size)) {}

		template <typename Byte>
		requires(sizeof(Byte) == 1) explicit bytes(
		    std::basic_string_view<Byte> const& view) noexcept
		    : bytes(view.data(), view.size()) {}

		constexpr std::byte const* data() const noexcept { return data_; }
		constexpr std::byte const* begin() const noexcept { return data_; }
		constexpr std::byte const* end() const noexcept {
			return data_ + length_;
		}
		constexpr size_type size() const noexcept { return length_; }

		bytes subview(size_t offset = 0) const noexcept {
			if (offset > length_) offset = length_;
			return {data_ + offset, length_ - offset};
		}

		bytes subview(size_t offset, size_t length) const noexcept {
			if (offset > length_) offset = length_;
			if ((length_ - offset) < length) length = length_ - offset;
			return {data_ + offset, length};
		}

	private:
		std::byte const* data_{};
		size_type length_{};
	};
}  // namespace git
