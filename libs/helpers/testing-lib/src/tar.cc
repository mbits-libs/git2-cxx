// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "tar.hh"
#include <fstream>

namespace testing::tar {
	using namespace std::filesystem;

	namespace {
		path make_path(std::string_view utf8) {
#ifdef __cpp_lib_char8_t
			return std::u8string_view{
			    reinterpret_cast<char8_t const*>(utf8.data()), utf8.length()};
#else
			return std::filesystem::u8path(utf8);
#endif
		}

	}  // namespace

	void unpack_files(std::filesystem::path const& root,
	                  std::span<std::string_view const> const& directories,
	                  std::span<file::text const> const& text_files,
	                  std::span<file::binary const> const& binary_files) {
		std::error_code ignore{};

		for (auto const subdir : directories) {
			create_directories(root / make_path(subdir), ignore);
		}

		for (auto const [filename, contents] : text_files) {
			auto const p = root / make_path(filename);
			create_directories(p.parent_path(), ignore);
			std::ofstream out{p, std::ios::binary};
			out.write(contents.data(),
			          static_cast<std::streamsize>(contents.size()));
		}

		for (auto const nfo : binary_files) {
			auto const p = root / make_path(nfo.path);
			create_directories(p.parent_path(), ignore);
			std::ofstream out{p, std::ios::binary};
			out.write(reinterpret_cast<char const*>(nfo.content.data()),
			          static_cast<std::streamsize>(nfo.content.size()));
		}
	}
}  // namespace testing::tar
