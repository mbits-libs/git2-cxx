// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <filesystem>
#include <string_view>

namespace testing::path {
	using std::filesystem::path;
#ifdef __cpp_lib_char8_t
	template <typename CharTo, typename Source>
	std::basic_string_view<CharTo> conv(Source const& view) {
		return {reinterpret_cast<CharTo const*>(view.data()), view.length()};
	}

	std::string get_path(path const& p) {
		auto const s8 = p.generic_u8string();
		auto const view = conv<char>(s8);
		return {view.data(), view.length()};
	}

	path make_u8path(std::string_view utf8) { return conv<char8_t>(utf8); }

#else
	std::string get_path(path const& p) { return p.generic_u8string(); }

	path make_u8path(std::string_view utf8) {
		return std::filesystem::u8path(utf8);
	}
#endif
}  // namespace testing::path

void PrintTo(std::filesystem::path const& path, ::std::ostream* os) {
	*os << testing::path::get_path(path);
}
