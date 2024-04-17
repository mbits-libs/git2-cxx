// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <compare>
#include <cov/git2/repository.hh>
#include <cov/git2/tree.hh>
#include <cov/io/report.hh>
#include <cov/io/types.hh>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace cov::app::report {
	enum class digest { unknown, md5, sha1 };

	struct file_info {
		using coverage_info =
		    std::tuple<std::vector<io::v1::coverage>, io::v1::coverage_stats>;

		struct function {
			std::string name{};
			std::string demangled_name{};
			uint32_t count{};
			io::v1::text_pos start{0, 0};
			io::v1::text_pos end{0, 0};

			bool operator==(function const&) const noexcept = default;

			auto operator<=>(function const& rhs) const noexcept {
				// 1. by position in file (ignore columns)
				// 2. by demangled
				// 3. by name

				if (auto const cmp = start.line <=> rhs.start.line; cmp != 0)
					return cmp;
				if (auto const cmp = end.line <=> rhs.end.line; cmp != 0)
					return cmp;
				if (auto const cmp = demangled_name <=> rhs.demangled_name;
				    cmp != 0)
					return cmp;
				return name <=> rhs.name;
			}
		};

		std::string name{};
		report::digest algorithm{digest::unknown};
		std::string digest{};
		std::map<unsigned, unsigned> line_coverage{};
		std::vector<function> function_coverage{};
		bool operator==(file_info const& rhs) const noexcept = default;
		auto operator<=>(file_info const& rhs) const noexcept {
			// without this definition, clang 16 does not like the
			// function_coverage' spaceship...
			if (auto const cmp = name <=> rhs.name; cmp != 0) {
				return cmp;
			}
			if (auto const cmp = algorithm <=> rhs.algorithm; cmp != 0) {
				return cmp;
			}
			if (auto const cmp = digest <=> rhs.digest; cmp != 0) {
				return cmp;
			}
			if (auto const cmp = line_coverage <=> rhs.line_coverage;
			    cmp != 0) {
				return cmp;
			}
			return function_coverage <=> rhs.function_coverage;
		}
		coverage_info expand_coverage(size_t line_count) const;
	};

	struct git_info {
		std::string branch{};
		std::string head{};
		auto operator<=>(git_info const&) const noexcept = default;
	};

	struct report_info {
		git_info git{};
		std::vector<file_info> files{};

		bool operator==(report_info const&) const noexcept = default;
		auto operator<=>(report_info const&) const noexcept = default;
		bool load_from_text(std::string_view u8_encoded);
	};

	struct git_signature {
		std::string name;
		std::string mail;
	};

	enum class text {
		missing = 0x0000'0000,
		in_repo = 0x0000'0001,
		in_fs = 0x0000'0002,
		different_newline = 0x0000'0004,
		mismatched = 0x0000'0008,
	};

	constexpr inline text operator|(text lhs, text rhs) {
		using underlying = std::underlying_type_t<text>;
		return static_cast<text>(static_cast<underlying>(lhs) |
		                         static_cast<underlying>(rhs));
	}

	constexpr inline text operator&(text lhs, text rhs) {
		using underlying = std::underlying_type_t<text>;
		return static_cast<text>(static_cast<underlying>(lhs) &
		                         static_cast<underlying>(rhs));
	}

	struct blob_info {
		text flags{text::missing};
		git::oid existing{};
		size_t lines{};
	};

	struct git_commit {
		git_signature author{};
		git_signature committer{};
		sys_seconds committed{};
		std::string message{};
		git::tree tree{};

		static git_commit load(git::repository_handle repo,
		                       std::string_view commit_id,
		                       std::error_code& ec);

		blob_info verify(file_info const& file) const;
	};
}  // namespace cov::app::report
