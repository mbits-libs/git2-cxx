// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <filesystem>
#include <span>
#include <string_view>

namespace testing::tar {
	namespace file {
		struct text {
			std::string_view path{};
			std::string_view content{};
		};

		struct binary {
			std::string_view path{};
			std::basic_string_view<unsigned char> content{};
		};
	}  // namespace file

	template <size_t Length>
	constexpr std::basic_string_view<unsigned char> span(
	    unsigned char const (&v)[Length]) noexcept {
		return {v, Length};
	}

	void unpack_files(std::filesystem::path const& root,
	                  std::span<std::string_view const> const& directories,
	                  std::span<file::text const> const& text_files,
	                  std::span<file::binary const> const& binary_files);
}  // namespace testing::tar

#define USE_TAR                                   \
	namespace {                                   \
		using namespace ::testing::tar;           \
		namespace file {                          \
			using namespace ::testing::tar::file; \
		}                                         \
	}
