// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/git2/config.hh>
#include <cov/git2/error.hh>
#include <cov/git2/repository.hh>
#include <cov/init.hh>
#include <cov/io/file.hh>
#include <optional>
#include <string>

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
			constexpr auto ref_prefix = "ref: "sv;
			constexpr auto objects_dir = "objects"sv;
			constexpr auto objects_pack_dir = "objects/pack"sv;
			constexpr auto coverage_dir = "objects/coverage"sv;
			constexpr auto refs_dir = "refs"sv;
			constexpr auto heads_dir = "refs/heads"sv;
			constexpr auto tags_dir = "refs/tags"sv;
			constexpr auto refs_dir_prefix = "refs/"sv;
			constexpr auto heads_dir_prefix = "refs/heads/"sv;
			constexpr auto tags_dir_prefix = "refs/tags/"sv;
			constexpr auto config = "config"sv;
			constexpr auto dot_config = ".covconfig"sv;
			constexpr auto HEAD = "HEAD"sv;
			constexpr auto covdir_link = "covdir"sv;
			constexpr auto commondir_link = "commondir"sv;
			constexpr auto gitdir_link = "gitdir"sv;

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
			auto true_base = base_dir;
			auto const in = io::fopen(base_dir / names::commondir_link, "rb");
			if (in) {
				auto const line = in.read_line();
				if (!line.empty()) {
					true_base = std::filesystem::weakly_canonical(
					    base_dir / make_path(line));
				}
			}

			if (!is_directory(true_base / names::coverage_dir, ec) || ec)
				return false;
			if (!is_regular_file(true_base / names::config, ec) || ec)
				return false;
			if (!is_regular_file(base_dir / names::HEAD, ec) || ec)
				return false;
			return true;
		}

		inline path rel_path(path const& dst, path const& srcdir) {
			auto src_abs = weakly_canonical(srcdir);
			auto dst_abs = weakly_canonical(dst);
			// GCOV_EXCL_START[POSIX]
			if (dst_abs.has_root_name() &&
			    dst_abs.root_name() == src_abs.root_name()) {
				dst_abs = "/"sv / dst_abs.relative_path();
			}
			// GCOV_EXCL_STOP
			auto result = proximate(dst, src_abs);
			auto rel_str = get_path(result);
			auto abs_str = get_path(dst_abs);
			auto rel_view = std::string_view{rel_str};
			auto abs_view = std::string_view{abs_str};

			if ((rel_view.length() > abs_view.length() &&
			     rel_view.substr(rel_view.length() - abs_view.length()) ==
			         abs_view)) {
				result = dst;
			}

			return result;
		}

		inline auto isspace(char c) {
			return std::isspace(static_cast<unsigned char>(c));
		}

		inline std::string_view lstrip(std::string_view view) {
			while (!view.empty() && isspace(view.front()))
				view = view.substr(1);
			return view;
		}

		inline std::string_view rstrip(std::string_view view) {
			while (!view.empty() && isspace(view.back()))
				view = view.substr(0, view.length() - 1);

			return view;
		}

		inline std::string_view strip(std::string_view view) {
			return rstrip(lstrip(view));
		}

		inline std::optional<std::string_view> prefixed(std::string_view prefix,
		                                                std::string_view line) {
			if (!line.starts_with(prefix)) return std::nullopt;
			return strip(line.substr(prefix.length()));
		}
	}  // namespace
};     // namespace cov
