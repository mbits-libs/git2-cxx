// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/io/types.hh>
#include <filesystem>
#include <git2/repository.hh>
#include <git2/tree.hh>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace cov::app::report {
	enum class digest { unknown, md5, sha1 };

	struct file_info {
		std::string name{};
		report::digest algorithm{digest::unknown};
		std::string digest{};
		std::map<unsigned, unsigned> line_coverage{};
		auto operator<=>(file_info const&) const noexcept = default;
	};

	struct git_info {
		std::string branch{};
		std::string head{};
		auto operator<=>(git_info const&) const noexcept = default;
	};

	struct report_info {
		git_info git{};
		std::vector<file_info> files{};

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
		git_oid existing{};
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

		blob_info verify(std::filesystem::path const& current_directory,
		                 file_info const& file) const;
	};
}  // namespace cov::app::report
