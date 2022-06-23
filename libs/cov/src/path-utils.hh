// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/init.hh>
#include <fstream>
#include <git2-c++/config.hh>
#include <git2-c++/error.hh>
#include <git2-c++/repository.hh>

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace cov {
	namespace {
		using namespace std::literals;
		using namespace std::filesystem;

		namespace names {
			constexpr auto covdata_dir = ".covdata"sv;
			constexpr auto covlink_prefix = "covdata: "sv;
			constexpr auto objects_dir = "objects"sv;
			constexpr auto objects_pack_dir = "objects/pack"sv;
			constexpr auto coverage_dir = "objects/coverage"sv;
			constexpr auto heads_dir = "refs/heads"sv;
			constexpr auto tags_dir = "refs/tags"sv;
			constexpr auto config = "config"sv;
			constexpr auto dotconfig = ".covconfig"sv;
			constexpr auto HEAD = "HEAD"sv;

			constexpr char core_gitdir[] = "core.gitdir";
		}  // namespace names

#ifdef __cpp_lib_char8_t
		template <typename CharTo, typename Source>
		inline std::basic_string_view<CharTo> conv(Source const& view) {
			return {reinterpret_cast<CharTo const*>(view.data()),
			        view.length()};
		}

		inline std::string get_path(path const& p) {
			auto const s8 = p.generic_u8string();
			auto const view = conv<char>(s8);
			return {view.data(), view.length()};
		}

		inline path make_path(std::string_view utf8) {
			return conv<char8_t>(utf8);
		}
#else
		inline std::string get_path(path const& p) {
			return p.generic_u8string();
		}
		inline path make_path(std::string_view utf8) { return u8path(utf8); }
#endif

		inline bool is_valid_path(path const& base_dir) {
			std::error_code ec{};
			if (!is_directory(base_dir / names::coverage_dir, ec) || ec)
				return false;
			if (!is_regular_file(base_dir / names::config, ec) || ec)
				return false;
			if (!is_regular_file(base_dir / names::HEAD, ec) || ec)
				return false;
			return true;
		}

		inline path rel_path(path const& dst, path const& srcdir) {
			auto result = proximate(dst, srcdir);
			auto rel_str = get_path(result);
			auto abs_str = get_path(dst);
			auto rel_view = std::string_view{rel_str};
			auto abs_view = std::string_view{abs_str};

			if (!rel_view.empty() &&
			    rel_view.back() == path::preferred_separator)
				rel_view = rel_view.substr(0, rel_view.length() - 1);
			if (!abs_view.empty() &&
			    abs_view.back() == path::preferred_separator)
				abs_view = abs_view.substr(0, abs_view.length() - 1);

			if (rel_view.length() > abs_view.length() &&
			    rel_view.substr(rel_view.length() - abs_view.length()) ==
			        abs_view) {
				result = dst;
			}

			return result;
		}
	}  // namespace
};     // namespace cov
